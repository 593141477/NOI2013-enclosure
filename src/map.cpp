#include <vector>
#include <cassert>
#include <algorithm>
#include <cstring>

#define LAND_MARK -2

static std::vector<Trap_t> Traps;

int LandOwner[MAP_WIDTH][MAP_HEIGHT];
int LandID[MAP_WIDTH][MAP_HEIGHT];
int totalEnclosure;

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
    fprintf(stderr, "not vertical or horizontal\n");
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
                // fprintf(stderr, "can't go from (%d,%d) to (%d,%d)\n", v.x, v.y, tx, ty);
                continue;
            }
            bfs_vst[tx][ty]  = 1;
            distance[tx][ty] = distance[v.x][v.y]+1;
            bfs_queue[r++] = Point_t(tx, ty);
        }
    }
}

bool inline smaller_and_update(int &a, const int b)
{
    if(b < a){
        a = b;
        return true;
    }
    return false;
}

std::pair<Point_t,int> find_nearest_blank(const int distance[MAP_WIDTH+1][MAP_HEIGHT+1])
{
    int best = 0x2f2f2f2f;
    Point_t p(0, 0);
    int corner = UL_CORNER;
    for(int i=0; i<MAP_WIDTH; i++)
        for(int j=0; j<MAP_HEIGHT; j++) {
            if(LandOwner[i][j] == LAND_NO_OWNER){
                if(smaller_and_update(best,distance[i][j])){ //upper left corner
                    p = Point_t(i, j);
                    corner = UL_CORNER;
                }
                if(smaller_and_update(best,distance[i+1][j+1])){ //lower right corner
                    p = Point_t(i+1, j+1);
                    corner = LR_CORNER;
                }
                if(smaller_and_update(best,distance[i+1][j])){ //upper right corner
                    p = Point_t(i+1, j);
                    corner = UR_CORNER;
                }
                if(smaller_and_update(best,distance[i][j+1])){ //lower left corner
                    p = Point_t(i, j+1);
                    corner = LL_CORNER;
                }
            }
        }
    fprintf(stderr, "%s: (%d,%d) dir:%d\n", __func__, p.x, p.y, corner);
    return std::make_pair(p, corner);
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

    // fprintf(stderr, "%s\n", "addEnclosure: ");
    // for(i=start; i!=end; i++)
    //  fprintf(stderr, "(%d,%d)", i->x, i->y);
    // fprintf(stderr, "\n");

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
    //  fprintf(stderr, "%s\n", "vt_lines: ");
    //  for(int i=0; i<len; i++)
    //      fprintf(stderr, "(%d, %d)\n", vt_lines[i].second, vt_lines[i].first);
    //  fprintf(stderr, "\n" );
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
    // fprintf(stderr, "======LandID======\n");
    // for(int i=0; i<MAP_HEIGHT; i++){
    //  for(int j=0; j<MAP_WIDTH; j++) {
    //      fprintf(stderr, "%02d, ", LandID[j][i]);
    //  }
    //  fprintf(stderr, "\n");
    // }
    fprintf(stderr, "======LandOwner======\n");
    for(int i=0; i<MAP_HEIGHT; i++){
        for(int j=0; j<MAP_WIDTH; j++) {
            fprintf(stderr, "%02d, ", LandOwner[j][i]);
        }
        fprintf(stderr, "\n");
    }
}

