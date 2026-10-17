"""Records Cataclysm II's catalog preview (hub/RECORDING.md): the first
screen with its "early alpha" notice, a Survivalist made in a few keys,
then the shipwreck beach. The driver wields the hatchet, looks at the
world map for the nearest town with zombies (drawn on red), walks there
and fights what comes. Usage, from the repo root after asciiverse/build.sh
and asciiverse/package.sh <target>, with pyte installed:

    python3 asciiverse/record/cataclysm2.py

The map is the left 30x30 of the screen, always centred on the player
(@ at column 15, row 15); the HUD is to its right. One world-map tile is
25x25 map tiles.
"""
import collections, json, os, random, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rec import Rec, ROOT, RIGHT, DOWN  # noqa: E402

MAPW = MAPH = 30
ME = (15, 15)
DIRS = {(0, -1): "k", (1, -1): "u", (1, 0): "l", (1, 1): "n",
        (0, 1): "j", (-1, 1): "b", (-1, 0): "h", (-1, -1): "y"}
MONSTERS = set("ZrdWcCb")
# what the player cannot walk through (data/terrain.dat): trees (7, 1),
# walls and boulders (#), fences and furniture ({), and tiles not seen
WALLS = set("71#{ ")


def grid_of(r):
    return [r.rows()[y][:MAPW] for y in range(MAPH)]


def blocked(r, x, y):
    ch = r.screen.buffer[y][x]
    if ch.data in WALLS:
        return True
    # deep water is plain cyan or blue, shallow water the bright ones
    if ch.data == "~" and ch.fg in ("cyan", "blue") and not ch.bold:
        return True
    if ch.data == "%" and ch.fg == "green" and not ch.bold:   # hedge
        return True
    return False


def town_offset(r):
    """On the world map: the nearest tile drawn on red, relative to the
    player's tile, in world tiles."""
    me = None
    reds = []
    for y in range(30):
        for x in range(80):
            c = r.screen.buffer[y][x]
            if c.fg == "black" and c.bg not in ("black", "default", "red"):
                me = (x, y)         # the player's tile, colours swapped
            elif c.bg == "red":
                reds.append((x, y))
    if not me or not reds:
        return None
    tx, ty = min(reds, key=lambda p: max(abs(p[0] - me[0]), abs(p[1] - me[1])))
    return tx - me[0], ty - me[1]


def bfs_step(r, goal_dir, avoid):
    """First step of a shortest path, over the visible map, to the seen
    cell that is furthest along goal_dir."""
    gx, gy = goal_dir
    start = ME
    prev = {start: None}
    q = collections.deque([start])
    best, best_score = None, None
    while q:
        p = q.popleft()
        score = (p[0] - ME[0]) * gx + (p[1] - ME[1]) * gy
        if p != start and (best_score is None or score > best_score):
            best, best_score = p, score
        for (dx, dy) in DIRS:
            n = (p[0] + dx, p[1] + dy)
            if not (0 <= n[0] < MAPW and 0 <= n[1] < MAPH) or n in prev:
                continue
            if blocked(r, *n) or n in avoid:
                continue
            prev[n] = p
            q.append(n)
    if best is None:
        return None
    p = best
    while prev[p] != start:
        p = prev[p]
    return (p[0] - ME[0], p[1] - ME[1])


def take(seed):
    random.seed(seed)
    r = Rec("cataclysm2", "Cataclysm II")
    r.wait_for("New Character", 8)
    r.pump(3.0)                     # the first screen: an early alpha
    r.key("N", 0.8)                 # stats: two points each to Str and Dex
    for k in (RIGHT, RIGHT, DOWN, RIGHT, RIGHT):
        r.key(k, 0.15)
    r.key(">", 0.6)                 # traits: none
    r.key(">", 0.8)                 # profession: Survivalist, the first
    r.key("\r", 0.6)
    r.key(">", 0.6)                 # name
    for k in "Ada":
        r.key(k, 0.2)
    r.key(">", 0.6)
    r.key("y", 0.5)
    r.wait_for("Weapon:", 10)
    r.pump(1.5)
    r.key("w", 0.8)                 # wield the hatchet
    r.key("f", 1.2)
    r.key(":", 1.0)                 # the world map
    off = town_offset(r)
    r.pump(1.5)
    r.key("q", 0.6)                 # q leaves the world map, Esc does not
    print("town at", off, file=sys.stderr)
    if off is None:
        off = (1, -1)
    target = [off[0] * 25, off[1] * 25]   # where the town is, in map tiles
    avoid = set()
    last = None
    while r.elapsed() < 54:
        text = r.text()
        if "Press spacebar" in text or "--more--" in text:
            r.key(" ", 0.3)
            continue
        grid = grid_of(r)
        # a monster next to us: hit it
        hit = None
        for (dx, dy), k in DIRS.items():
            x, y = ME[0] + dx, ME[1] + dy
            if grid[y][x] in MONSTERS:
                hit = k
                break
        if hit:
            r.key(hit, 0.35)
            continue
        # a monster in sight: go for it
        mons = [(x, y) for y in range(MAPH) for x in range(MAPW) if grid[y][x] in MONSTERS]
        if mons:
            mx, my = min(mons, key=lambda p: max(abs(p[0] - 15), abs(p[1] - 15)))
            goal = (mx - 15, my - 15)
        else:
            goal = (target[0], target[1])
        n = max(abs(goal[0]), abs(goal[1]), 1)
        step = bfs_step(r, (goal[0] / n, goal[1] / n), avoid)
        if step is None:
            step = random.choice(list(DIRS))
        before = grid
        r.key(DIRS[step], 0.12 if not mons else 0.25)
        if grid_of(r) == before:
            if not mons:
                avoid.add((ME[0] + step[0], ME[1] + step[1]))
        else:                       # we moved: the town is a step nearer
            avoid.clear()
            target[0] -= step[0]
            target[1] -= step[1]
    r.key("Q", 0.8)                 # quit: the game asks first
    r.key("y", 1.5)
    r.finish(b"", grace=4.0)


def score(cast):
    """Recorded length (idle limit applied) and the number of hits and
    kills seen in it."""
    lines = open(cast).read().splitlines()
    idle = json.loads(lines[0]).get("idle_time_limit") or 2
    length = sum(min(json.loads(l)[0], idle) for l in lines[1:])
    text = "".join(json.loads(l)[2] for l in lines[1:] if json.loads(l)[1] == "o")
    return length, text.count("You hit") + 3 * text.count("You kill")


def main():
    """Best of a few takes: the world is new every time, and some starts
    are far from a town."""
    cast = os.path.join(ROOT, "asciiverse", "casts", "cataclysm2.cast")
    best = None
    for attempt in range(8):
        take(attempt)
        length, fights = score(cast)
        print(f"take {attempt + 1}: {length:.0f} s, fight score {fights}", file=sys.stderr)
        if 35 <= length <= 60 and (best is None or fights > best[0]):
            best = (fights, open(cast).read())
        if best and best[0] >= 40:
            break
    if best:
        with open(cast, "w") as f:
            f.write(best[1])


if __name__ == "__main__":
    main()
