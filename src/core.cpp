#include <cstdio>
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
                fprintf(stderr, "%s: from (%d,%d)to(%d,%d)\n", __func__,
                    x, y, tx, ty);
                if(!distance[tx][ty]){
                    ret = DELTA_NAME[DELTA_INVERSE[i]];
                }
                x = tx;
                y = ty;
                break;
            }
        }
    }
    return ret;
}
//从某个点开始,按某方向走一步后所在的点
Point_t getNextPoint(const Point_t &st, int Dir)
{
    Point_t next;
    for(int i=0; i<4; i++)
        if(DELTA_NAME[i]==Dir)
            next = Point_t(st.x+DELTA[i][0], st.y+DELTA[i][1]);
    return next;
}
bool anyBotThere(const Point_t &pt)
{
    const BotsInfo_t& bots = get_bots_info();
    for(int i=0; i<NUM_PLAYERS; i++)
        if(bots.status[i]!=BOT_DEAD && bots.pos[i]==pt)
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
int NearestEnemyDist(int me)
{
    const BotsInfo_t& bots = get_bots_info();
    const std::vector<Point_t> track = getTrack(me);
    int dist[MAP_WIDTH+1][MAP_HEIGHT+1];
    int shortest = 0x2f2f2f2f;
    for(int i=0; i<NUM_PLAYERS; i++)
    {
        if(bots.status[i]==BOT_DEAD || i==me)
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
bool inDangerNow(int me, const Point_t &start)
{
    int dist[MAP_WIDTH+1][MAP_HEIGHT+1];
    const std::vector<Point_t> track = getTrack(me);
    int length = track.size();

    if(length < 2) /* shouldn't be that */
        return false;
    BfsSearch(track[length-1].x, track[length-1].y,
            LAND_NO_OWNER, dist, track[length-2]);
    int nearest = NearestEnemyDist(me);
    int remaining = dist[start.x][start.y];

    fprintf(stderr, "%s: nearest=%d remaining=%d\n", __func__, nearest, remaining);

    return nearest < remaining;
}
Point_t chooseEscDest(int esc_dist[MAP_WIDTH+1][MAP_HEIGHT+1], int me)
{
    const std::vector<Point_t> track = getTrack(me);
    int length = track.size();
    BfsSearch(track[length-1].x, track[length-1].y,
            LAND_NO_OWNER, esc_dist, track[length-2]);
    int nearest = NearestEnemyDist(me);

    Point_t ret = track[0];
    int shortest = 0x2f2f2f2f;
    for(std::vector<Point_t>::const_iterator it=track.begin();
            it != track.end(); ++it) {
        int d = esc_dist[it->x][it->y];
        if(d!=0 && smaller_and_update(shortest, d)) {
            fprintf(stderr, "%s: nearest=%d shortest=%d\n", __func__, nearest, shortest);
            ret = *it;
            if(shortest <= nearest)
                break;
        }
    }
    return ret;
}
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
        fprintf(stderr, "%s unknown dir\n", __func__);
    }
    return ret;
}
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
        fprintf(stderr, "%s unknown dir\n", __func__);
    }
    return ret;
}