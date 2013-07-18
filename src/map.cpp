#include <vector>
#include <cassert>
#include <algorithm>
#include <cstring>

#define LAND_MARK -2

static std::vector<Trap_t> Traps;

int LandOwner[MAP_WIDTH][MAP_HEIGHT];
int LandID[MAP_WIDTH][MAP_HEIGHT];
int totalEnclosure;


int BlankSize[MAP_WIDTH][MAP_HEIGHT];

//所有格点之间的最短路
// int Graph[MAX_VERTICES][MAX_VERTICES+7 /*128*/];
// #define COORD2VERT(x,y) ((x)*(MAP_WIDTH+1)+(y))

// void update_graph()
// {
//  memset(Graph, 0x2f, sizeof Graph);
//  for(int i=0; i<=MAP_WIDTH; i++){
//      for(int j=0; j<=MAP_HEIGHT; j++){
//          //TODO: 区分能否穿越自己区域的最短路
//          int st = COORD2VERT(i, j);
//      }
//  }
// }

int getEdgeOwner(const Point_t &st, const Point_t &ed)
{
    if(st.x == ed.x) { //vertical
        int rx,lx,ry,ly;
        rx = st.x;
        lx = rx-1;
        ry = ly = std::min(ed.y, st.y);
        if(lx<0 || rx>=MAP_WIDTH)
            return LAND_NO_OWNER;
        if(LandID[lx][ly] != LandID[rx][ry])
            return LAND_NO_OWNER;
        return LandOwner[lx][ly];
    }else if(st.y == ed.y) { //horizontal
        int dx,dy,ux,uy;
        dy = st.y;
        uy = dy-1;
        dx = ux = std::min(ed.x, st.x);
        if(uy<0 || dy>=MAP_HEIGHT)
            return LAND_NO_OWNER;
        if(LandID[dx][dy] != LandID[ux][uy])
            return LAND_NO_OWNER;
        return LandOwner[dx][dy];
    }
    dbgprint(stderr, "not vertical or horizontal\n");
    assert(0);
}

bool bfs_vst[MAP_WIDTH+1][MAP_HEIGHT+1];
Point_t bfs_queue[MAX_VERTICES+7];
void BfsSearch(int x, int y, int player, 
    int distance[MAP_WIDTH+1][MAP_HEIGHT+1],
    Point_t lastPt)
{
    memset(distance, 0x2f, MAX_VERTICES*sizeof(int));
    memset(bfs_vst, 0, sizeof(bfs_vst));

    Point_t src(x, y);

    int l=0, r=0;
    bfs_queue[r++] = src;
    bfs_vst[x][y] = 1;
    distance[x][y] = 0;
    while(l!=r){
        Point_t v = bfs_queue[l++];
        for(int i=0; i<4; i++){
            int tx = v.x + DELTA[i][0];
            int ty = v.y + DELTA[i][1];
            Point_t t(tx,ty);
            if(v==src && lastPt==t)
                continue;
            if(t.OutOfBounds() || bfs_vst[tx][ty])
                continue;
            int owner = getEdgeOwner(v, t);
            if(owner!=LAND_NO_OWNER && owner!=player){
                // dbgprint(stderr, "can't go from (%d,%d) to (%d,%d)\n", v.x, v.y, tx, ty);
                continue;
            }
            bfs_vst[tx][ty]  = 1;
            distance[tx][ty] = distance[v.x][v.y]+1;
            bfs_queue[r++] = Point_t(tx, ty);
        }
    }
}

//各个点的受威胁程度高低,值越大越威胁
int enemyDistanceMap[MAP_WIDTH+1][MAP_HEIGHT+1];
int (*get_distance_map())[MAP_HEIGHT+1]
{
    return enemyDistanceMap;
}
void debug_print_dm()
{
    dbgprint(stderr, "======enemyDistanceMap======\n");
    for(int i=0; i<=MAP_HEIGHT; i++){
        for(int j=0; j<=MAP_WIDTH; j++) {
            dbgprint(stderr, "%d, ", enemyDistanceMap[j][i]);
        }
        dbgprint(stderr, "\n");
    }
}
inline int square(int x)
{
    return x*x*x;
}
void drawDistanceMap(const BotsInfo_t &info)
{
    memset(enemyDistanceMap, 0, sizeof enemyDistanceMap);
    for(int x=0; x<=MAP_WIDTH; x++)
        for(int y=0; y<=MAP_HEIGHT; y++) {
            Point_t pt(x,y);
            for(int i=0; i<NUM_PLAYERS; i++){
                if(info.status[i]==BOT_DEAD || info.MyID==i)
                    continue;

                enemyDistanceMap[x][y] = std::max(
                    enemyDistanceMap[x][y],
                    square(20 - pt.dist(info.pos[i]))
                );
                // enemyDistanceMap[x][y] += square(20 - pt.dist(info.pos[i]));
            }
        }
    // debug_print_dm();
}

bool inline smaller_and_update(int &a, const int b)
{
    if(b < a){
        a = b;
        return true;
    }
    return false;
}

int get_max_square(int x, int y)
{
    int ans;
    for(ans=0; ; ans++) {
        int sx = x-ans, sy = y-ans;
        int tx = x+ans, ty = y+ans;

        if(sx<0 || sy<0 || tx>=MAP_WIDTH || ty>=MAP_HEIGHT)
            break;
        for(int i=sx; i<=tx; i++)
            for(int j=sy; j<=ty; j++)
                if(LandOwner[i][j]!=LAND_NO_OWNER)
                    goto finished;
    }
finished:
    // dbgprint(stderr, "%s: (%d,%d)=%d\n", __func__, x, y, ans);
    return ans;
}
int tmp_turn_seq[]={1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,19,20,20};
int count_blank_blocks(int x, int y, int dir)
{
    int cnt = 0;
    int times = 0, p = 0;
    while(x>=0 && x<MAP_WIDTH && y>=0 && y<MAP_HEIGHT && LandOwner[x][y]==LAND_NO_OWNER){
        cnt++;
        // dbgprint(stderr, "%s: %d @ (%d,%d)\n", __func__, cnt, x, y);
        for(int i=0; i<4; i++)
            if(DELTA_NAME[i]==dir){
                x += DELTA[i][0];
                y += DELTA[i][1];
            }
        times++;
        if(times == tmp_turn_seq[p]){
            p++;
            times=0;
            dir=ClockwiseTurn(dir);
        }
    }
    return cnt;
}
Point_t find_nearest_blank(const int distance[MAP_WIDTH+1][MAP_HEIGHT+1], int &ret_cor, int &ret_size, int min_size)
{
    const int max_length=MAP_WIDTH*MAP_HEIGHT;
    int dists[max_length+1];
    Point_t points[max_length+1];
    int corners[max_length+1];
    int sizes[max_length+1];

    memset(dists, 0x2f, sizeof dists);
    for(int i=0; i<MAP_WIDTH; i++)
        for(int j=0; j<MAP_HEIGHT; j++) {
            if(LandOwner[i][j]==LAND_NO_OWNER && BlankSize[i][j]>=min_size){
                //按照可扩展范围,分别得到最优解
                int sp = count_blank_blocks(i,j,DIR_LEFT);
                if(smaller_and_update(dists[sp],distance[i][j])){ //upper left corner
                    points[sp] = Point_t(i, j);
                    corners[sp] = UL_CORNER;
                    sizes[sp] = BlankSize[i][j]; 
                }
                sp = count_blank_blocks(i,j,DIR_RIGHT);
                if(smaller_and_update(dists[sp],distance[i+1][j+1])){ //lower right corner
                    points[sp] = Point_t(i+1, j+1);
                    corners[sp] = LR_CORNER;
                    sizes[sp] = BlankSize[i][j]; 
                }
                sp = count_blank_blocks(i,j,DIR_UP);
                if(smaller_and_update(dists[sp],distance[i+1][j])){ //upper right corner
                    points[sp] = Point_t(i+1, j);
                    corners[sp] = UR_CORNER;
                    sizes[sp] = BlankSize[i][j]; 
                }
                sp = count_blank_blocks(i,j,DIR_DOWN);
                if(smaller_and_update(dists[sp],distance[i][j+1])){ //lower left corner
                    points[sp] = Point_t(i, j+1);
                    corners[sp] = LL_CORNER;
                    sizes[sp] = BlankSize[i][j]; 
                }
            }
        }
    int shortest = * std::min_element(dists+1, dists+max_length+1);
    for(int i=max_length/5; i>=1; i--){
        if(dists[i]-shortest < 5){
            const Point_t &p = points[i];
            ret_size = sizes[i];
            ret_cor = corners[i];
            dbgprint(stderr, "%s: (%d,%d) dir:%d size:%d\n", __func__, p.x, p.y, ret_cor, ret_size);
            return p;
        }
    }

    dbgprint(stderr, "nothing to return in %s\n", __func__);
    assert(0);
    return points[0];
}
std::pair<Point_t,int> find_best_blank(const int distance[MAP_WIDTH+1][MAP_HEIGHT+1])
{
    int corner, blank_size;
    Point_t p = find_nearest_blank(distance, corner, blank_size, 0);
    if(blank_size < 4){
        int largest = getLargestBlankSize();
        p = find_nearest_blank(distance, corner, blank_size, largest);
    }
    return std::make_pair(p, corner);
}

bool blankSizeVst[MAP_WIDTH][MAP_HEIGHT];
int getLargestBlankSize()
{
    int ret = 0;
    for(int i=0; i<MAP_WIDTH; i++)
        for(int j=0; j<MAP_HEIGHT; j++)
            if(BlankSize[i][j] > ret)
                ret = BlankSize[i][j];
    return ret;
}
int cntBlank(int x, int y)
{
    if(x<0 || y<0 || x>=MAP_WIDTH || y>=MAP_HEIGHT || blankSizeVst[x][y] || LandOwner[x][y]!=LAND_NO_OWNER)
        return 0;
    blankSizeVst[x][y] = 1;
    int ret = 1;
    for(int i=0; i<4; i++)
        ret += cntBlank(x+DELTA[i][0], y+DELTA[i][1]);
    return ret;
}
void doUpdatingBlank(int x, int y, int val)
{
    if(x<0 || y<0 || x>=MAP_WIDTH || y>=MAP_HEIGHT || BlankSize[x][y]!=-1 || LandOwner[x][y]!=LAND_NO_OWNER)
        return ;
    BlankSize[x][y] = val;
    for(int i=0; i<4; i++)
        doUpdatingBlank(x+DELTA[i][0], y+DELTA[i][1], val);
}
void updateBlankSize()
{
    memset(BlankSize, -1, sizeof BlankSize);
    memset(blankSizeVst, 0, sizeof blankSizeVst);
    for(int i=0; i<MAP_WIDTH; i++)
        for(int j=0; j<MAP_HEIGHT; j++){
            if(LandOwner[i][j] == LAND_NO_OWNER){
                int cnt = cntBlank(i,j);
                doUpdatingBlank(i,j,cnt);
            }else{
                BlankSize[i][j]=0;
            }
        }
    // dbgprint(stderr, "%s\n", "=====BlankSize=====");
    // for(int i=0; i<MAP_HEIGHT; i++){
    //     for(int j=0; j<MAP_WIDTH; j++){
    //         dbgprint(stderr, "%d, ", BlankSize[j][i]);
    //     }
    //     dbgprint(stderr, "\n");
    // }
}

void init_map()
{
    memset(LandOwner, LAND_NO_OWNER, sizeof LandOwner);
    memset(LandID, LAND_NO_OWNER, sizeof LandID);

    // update_graph();
}

void read_Traps()
{
    int n;
    Traps.clear();
    std::cin>>n;
    for(int i=0; i<n; i++){
        Trap_t tmp;
        std::cin >> tmp.x >> tmp.y >> tmp.left;
        Traps.push_back(tmp);
    }
}
struct enclosure_t{
    std::vector<Point_t> blocks;
    int player;

    //compare area
    bool operator<(const enclosure_t& t)const{
        return blocks.size() < t.blocks.size();
    }
};
static std::vector<enclosure_t> pendingEnclosures;
void addEnclosure(const std::vector<Point_t>::iterator &start,
                const std::vector<Point_t>::iterator &end, 
                int player)
{
    std::vector<std::pair<int,int> > vt_lines;
    std::vector<Point_t>::iterator last, i;

    // dbgprint(stderr, "%s\n", "addEnclosure: ");
    // for(i=start; i!=end; i++)
    //  dbgprint(stderr, "(%d,%d)", i->x, i->y);
    // dbgprint(stderr, "\n");

    last = i = start;
    for(++i; i!=end; ++i,++last) {
        if(i->x != last->x)
            continue;
        vt_lines.push_back(std::make_pair(
            std::min(i->y, last->y), last->x
        ));
    }
    assert(vt_lines.size()%2 == 0);
    std::sort(vt_lines.begin(), vt_lines.end());

    enclosure_t tmp;
    int len = vt_lines.size();
    // if(len){
    //  dbgprint(stderr, "%s\n", "vt_lines: ");
    //  for(int i=0; i<len; i++)
    //      dbgprint(stderr, "(%d, %d)\n", vt_lines[i].second, vt_lines[i].first);
    //  dbgprint(stderr, "\n" );
    // }
    for(int i=0; i<len; i+=2) {
        for(int j=vt_lines[i].second; j<vt_lines[i+1].second; j++){
            tmp.blocks.push_back(Point_t(j, vt_lines[i].first));
        }
    }
    tmp.player = player;
    pendingEnclosures.push_back(tmp);
}
void floodfill(int x, int y, int id)
{
    if(x<0 || y<0 || x>=MAP_WIDTH || y>=MAP_HEIGHT || LandID[x][y]!=LAND_MARK)
        return;
    LandID[x][y] = id;
    for(int i=0; i<4; i++)
        floodfill(x+DELTA[i][0], y+DELTA[i][1], id);
}
void doEnclose()
{
    std::sort(pendingEnclosures.begin(), pendingEnclosures.end());
    std::vector<enclosure_t>::iterator it;
    for (it=pendingEnclosures.begin(); it!=pendingEnclosures.end(); ++it) {
        std::vector<Point_t> &blocks = (*it).blocks;
        std::vector<Point_t>::iterator p;
        for(p=blocks.begin(); p!=blocks.end(); p++) {
            if(LandOwner[p->x][p->y] == LAND_NO_OWNER){
                LandOwner[p->x][p->y] = (*it).player;
                LandID[p->x][p->y] = LAND_MARK;
            }
        }
        for(p=blocks.begin(); p!=blocks.end(); p++) {
            if(LandID[p->x][p->y] == LAND_MARK){
                floodfill(p->x, p->y, totalEnclosure);
                totalEnclosure++;
            }
        }
    }
    // if(!pendingEnclosures.empty())
    //  update_graph();
    pendingEnclosures.clear();
}

static std::vector<Point_t> Track[NUM_PLAYERS];
void update_map(const BotsInfo_t &info)
{
    for(int i=0; i<NUM_PLAYERS; i++) {
        if(info.status[i] == BOT_DRAWING){
            
            if(Track[i].empty())
                Track[i].push_back(info.last_pos[i]);
            if(! (*(Track[i].rbegin()) == info.pos[i]))
                Track[i].push_back(info.pos[i]);

        }else if(info.status[i] != BOT_DRAWING && !Track[i].empty()) {

            std::vector<Point_t>::iterator it;
            it = std::find(Track[i].begin(), Track[i].end(), info.pos[i]);
            if(it != Track[i].end()){
                if(!(info.pos[i]==*Track[i].rbegin())){ //确保不是非法操作
                    Track[i].push_back(info.pos[i]);
                    it = std::find(Track[i].begin(), Track[i].end(), info.pos[i]);
                    addEnclosure(it, Track[i].end(), i);
                }
                Track[i].clear();
            }
        }
    }
    doEnclose();
    drawDistanceMap(info);
    updateBlankSize();
}

bool onTheTrack(const Point_t &p, int who)
{
    return Track[who].end() !=
        std::find(Track[who].begin(), Track[who].end(), p);
}

const std::vector<Point_t>& getTrack(int who)
{
    return Track[who];
}

void debug_print_land()
{
    // dbgprint(stderr, "======LandID======\n");
    // for(int i=0; i<MAP_HEIGHT; i++){
    //  for(int j=0; j<MAP_WIDTH; j++) {
    //      dbgprint(stderr, "%02d, ", LandID[j][i]);
    //  }
    //  dbgprint(stderr, "\n");
    // }
    dbgprint(stderr, "======LandOwner======\n");
    for(int i=0; i<MAP_HEIGHT; i++){
        for(int j=0; j<MAP_WIDTH; j++) {
            dbgprint(stderr, "%02d, ", LandOwner[j][i]);
        }
        dbgprint(stderr, "\n");
    }
}

