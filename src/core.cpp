#include <cstdio>
#include <cassert>
#include <vector>

//根据目标位置取得下一步的方向
int calc_next_step(const Point_t &dest,
        int distance[MAP_WIDTH+1][MAP_HEIGHT+1],
        int player,
        Point_t lastPt)
{
    int ret = DIR_STOP;
    int x = dest.x, y = dest.y;

    //从目标位置反向寻找一条距离递减的路径
    while(distance[x][y]){
        std::vector<std::pair<Point_t,int> > acceptable;
        for(int i=0; i<4; i++){
            int tx = x+DELTA[i][0];
            int ty = y+DELTA[i][1];
            Point_t t(tx, ty);

            if(t.OutOfBounds())
                continue;
            //禁止倒退
            if(!distance[tx][ty] && x==lastPt.x && y==lastPt.y)
                continue;

            int owner = getEdgeOwner(Point_t(x,y), t);
            if(owner!=LAND_NO_OWNER && owner!=player)
                continue;
            if(distance[tx][ty] < distance[x][y]){
                acceptable.push_back(std::make_pair(t, DELTA_NAME[DELTA_INVERSE[i]]));
            }
        }
        if(acceptable.empty()){
            dbgprint(stderr, "no way to go to (%d,%d)\n", dest.x, dest.y);
            assert(0);
        }
        //随机选择一条可行路径
        const std::pair<Point_t,int> &Pair = acceptable[my_rand() % acceptable.size()];
        const Point_t &pt = Pair.first;
        dbgprint(stderr, "%s: from (%d,%d) to (%d,%d)\n", __func__,
            x, y, pt.x, pt.y);
        if(!distance[pt.x][pt.y]){
            ret = Pair.second;
        }
        x = pt.x;
        y = pt.y;
    }
    return ret;
}
//从某个点开始,按某方向走一步后所在的点
Point_t getNextPoint(const Point_t &st, int Dir)
{
    Point_t next = st;
    for(int i=0; i<4; i++)
        if(DELTA_NAME[i]==Dir)
            next = Point_t(st.x+DELTA[i][0], st.y+DELTA[i][1]);
    return next;
}
bool anyBotThere(const Point_t &pt)
{
    const BotsInfo_t& bots = get_bots_info();
    for(int i=0; i<NUM_PLAYERS; i++)
        if(bots.status[i]!=BOT_DEAD && bots.status[i]!=BOT_DRAWING && bots.pos[i]==pt)
            return true;
    return false;
}
bool canGoInto(const Point_t &pos, int Dir)
{
    Point_t next = getNextPoint(pos, Dir);
    if(next.OutOfBounds())
        return false;
    if(anyBotThere(next))
        return false;
    int o = getEdgeOwner(pos, next);
    return (o==LAND_NO_OWNER);
}
//找到能攻击我的最近的敌人
int NearestEnemyDist(const BotsInfo_t& bots, const std::vector<Point_t> &track )
{
    int dist[MAP_WIDTH+1][MAP_HEIGHT+1];
    int shortest = 0x2f2f2f2f;
    for(int i=0; i<NUM_PLAYERS; i++)
    {
        if(bots.status[i]==BOT_DEAD || bots.status[i]==BOT_DRAWING || i==bots.MyID)
            continue;
        //得到其他bot攻击到我的距离(处于落笔态的不能穿越自己的区域)
        BfsSearch(bots.pos[i].x, bots.pos[i].y,
            (bots.status[i]==BOT_NORMAL ? i : LAND_NO_OWNER), dist);
        for(std::vector<Point_t>::const_iterator it=track.begin();
            it != track.end(); ++it) {

            smaller_and_update(shortest, dist[it->x][it->y]);
        }
    }
    return shortest;
}
//比较最近的敌人和我落笔的距离，判断当前是否处于危险状态
bool inDangerNow(const Point_t &start)
{
    const BotsInfo_t& bots = get_bots_info();
    int dist[MAP_WIDTH+1][MAP_HEIGHT+1];
    const std::vector<Point_t> &track = getTrack(bots.MyID);
    int length = track.size();

    if(length < 2) /* shouldn't be that */
        return false;
    BfsSearch(track[length-1].x, track[length-1].y,
            LAND_NO_OWNER, dist, track[length-2]);
    int nearest = NearestEnemyDist(bots, track);
    int remaining = dist[start.x][start.y];
    for(int i=length-2; i>=0; i--)
        smaller_and_update(remaining, dist[track[i].x][track[i].y]);

    dbgprint(stderr, "%s: nearest=%d remaining=%d\n", __func__, nearest, remaining);

    return nearest <= remaining;
}
//寻找一个合适的落笔地点
Point_t chooseEscDest(int esc_dist[MAP_WIDTH+1][MAP_HEIGHT+1])
{
    const BotsInfo_t& bots = get_bots_info();
    const std::vector<Point_t> &track = getTrack(bots.MyID);
    int length = track.size();
    BfsSearch(track[length-1].x, track[length-1].y,
            LAND_NO_OWNER, esc_dist, track[length-2]);
    int nearest = NearestEnemyDist(bots, track);

    Point_t ret = track[0];
    int shortest = 0x2f2f2f2f;
    for(std::vector<Point_t>::const_iterator it=track.begin();
            it != track.end(); ++it) {
        int d = esc_dist[it->x][it->y];
        if(d!=0 && smaller_and_update(shortest, d)) {
            dbgprint(stderr, "%s: nearest=%d shortest=%d\n", __func__, nearest, shortest);
            ret = *it;
            if(shortest <= nearest)
                break;
        }
    }
    return ret;
}
//根据敌人分布的密度图，寻找空地
void getUncrowded(Point_t &p, int dist[MAP_WIDTH+1][MAP_HEIGHT+1])
{
    int (*DM)[MAP_HEIGHT+1] = get_distance_map(), n=0, best=0x2f2f2f2f;
    std::pair<int, std::pair<int,int> > vals[MAX_VERTICES];
    for(int i=0; i<=MAP_WIDTH; i++)
        for(int j=0; j<=MAP_HEIGHT; j++) {
            vals[n++]=std::make_pair(DM[i][j], std::make_pair(i,j));
        }
    std::sort(vals, vals+n);
    for(int i=0; i<=(n/10); i++) {
        int x = vals[i].second.first;
        int y = vals[i].second.second;
        if(best>dist[x][y] || best==dist[x][y] && (my_rand()&1)){
            best=dist[x][y];
            p.x = x;
            p.y = y;
        }
    }
    dbgprint(stderr, "%s: (%d,%d) val=%d\n", __func__, p.x, p.y, DM[p.x][p.y]);
}
//判断是否达到空地
bool uncrowdedEnough(const Point_t &p)
{
    int (*DM)[MAP_HEIGHT+1] = get_distance_map();
    int val = DM[p.x][p.y], cnt=0;
    for(int i=0; i<=MAP_WIDTH; i++)
        for(int j=0; j<=MAP_HEIGHT; j++) 
            if(DM[i][j] < val)
                cnt++;
    return (cnt <= MAX_VERTICES/9);
    // return val < 30;
}
//检测能否攻击敌人
int canAttack()
{
    const BotsInfo_t& Bots = get_bots_info();
    for (int i = 0; i < NUM_PLAYERS; ++i){
        if(i==Bots.MyID || Bots.status[i]==BOT_DEAD)
            continue;
        for(int j=0; j<4; j++){
            Point_t target(Bots.pos[Bots.MyID].x+DELTA[j][0], Bots.pos[Bots.MyID].y+DELTA[j][1]);
            if(!target.OutOfBounds() && onTheTrack(target, i) && canGoInto(Bots.pos[Bots.MyID], DELTA_NAME[j])){
                dbgprint(stderr, "%s\n", "attack");
                return DELTA_NAME[j];
            }
        }
    }
    return -1;
}
//计算顺时针转的位置
int ClockwiseTurn(int dir)
{
    int ret;
    switch(dir){
        case DIR_UP:
        ret = DIR_RIGHT;
        break;

        case DIR_RIGHT:
        ret = DIR_DOWN;
        break;

        case DIR_DOWN:
        ret = DIR_LEFT;
        break;

        case DIR_LEFT:
        ret = DIR_UP;
        break;

        default:
        ret = DIR_STOP;
        dbgprint(stderr, "%s unknown dir\n", __func__);
    }
    return ret;
}
//计算逆时针转的位置
int AntiClockwiseTurn(int dir)
{
    int ret;
    switch(dir){
        case DIR_UP:
        ret = DIR_LEFT;
        break;

        case DIR_RIGHT:
        ret = DIR_UP;
        break;

        case DIR_DOWN:
        ret = DIR_RIGHT;
        break;

        case DIR_LEFT:
        ret = DIR_DOWN;
        break;

        default:
        ret = DIR_STOP;
        dbgprint(stderr, "%s unknown dir\n", __func__);
    }
    return ret;
}