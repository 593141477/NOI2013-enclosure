#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#if !defined(_WIN32) && defined(DEBUG_BOT)
#include <unistd.h>
#endif 

int my_rand()
{
#ifdef DEBUG_NO_RAND
    // static unsigned int seed = 19921234;
    // return (seed = seed * 1103515245 + 12345) & ((1<<16)-1);
    return 0;
#else
    return rand();
#endif
}
void debug_print_time_usage()
{
    static unsigned long long last_stamp = 0;
    unsigned long long now = clock();
    dbgprint(stderr, "%s: %f\n","time usage", (now-last_stamp)/(float)CLOCKS_PER_SEC);
    last_stamp = now;
}

void init_program(int entropy)
{
    srand((time(0)^1995) + entropy);
#if !defined(_WIN32) && defined(DEBUG_BOT)
    srand(rand()+getpid());
#endif

    init_map();
}

void output_init_pos()
{
    std::cout << "[POS] " << 
        my_rand()%(MAP_WIDTH/4)+MAP_WIDTH/2 << ' ' << 
        my_rand()%(MAP_HEIGHT/4)+MAP_HEIGHT/2 << std::endl;
}
bool read_situation(BotsInfo_t &Bots)
{
    std::string tmp;
    if(!(std::cin >> tmp))
        return false;
    for(int i=0; i<NUM_PLAYERS; i++) {
        Bots.last_pos[i] = Bots.pos[i];
        std::cin >> Bots.pos[i].x >> Bots.pos[i].y;
        std::cin >> Bots.status[i] >> Bots.trapped[i] >> Bots.scoredecline[i];
    }
    read_Traps();
    return true;
}
void debug_print_distance(int distance[MAP_WIDTH+1][MAP_HEIGHT+1])
{
    dbgprint(stderr, "======distance======\n");
    for(int i=0; i<=MAP_HEIGHT; i++){
        for(int j=0; j<=MAP_WIDTH; j++){
            if(distance[j][i] == 0x2f2f2f2f)
                dbgprint(stderr, "%s, ", "oo");
            else
                dbgprint(stderr, "%02d, ", distance[j][i]);
        }
        dbgprint(stderr, "\n");
    }
}

BotsInfo_t Bots;
const BotsInfo_t& get_bots_info()
{
    return Bots;
}

int main()
{
    int MyBotId;
    int Round;
    std::string tmp;
    int distance[MAP_WIDTH+1][MAP_HEIGHT+1];
    Point_t startPoint, lastPoint;
    int lastDir;

    int state = STATE_FIND_UNCROWDED;

    std::cin >> tmp >> MyBotId;

    init_program(MyBotId);
    output_init_pos();

    Bots.MyID = MyBotId;

    for(Round=1; Round<=100; Round++) {
        char output_dir;
        int output_action;
        if(!read_situation(Bots))
            break;
        update_map(Bots);

        if(Bots.trapped[MyBotId]){
            output_action = ACTION_NOTHING;
            output_dir = DIR_STOP;
        }else{
            const Point_t &MyPosNow = Bots.pos[MyBotId];

            BfsSearch(MyPosNow.x, MyPosNow.y,
                MyBotId, distance);
            // debug_print_distance(distance);

            output_action = ACTION_NOTHING;
            output_dir = DIR_STOP;

            switch(state) {
                case STATE_ENCLOSING:
                {
                    if(Bots.status[MyBotId] == BOT_NORMAL)
                        state = STATE_FIND_LAND;
                    else if(inDangerNow(startPoint)){
                        dbgprint(stderr, "%s\n", "start escaping...");
                        state = STATE_ESCAPE;
                    }else if(Round == 99){ 
                        //接近最后回合直接结束圈地
                        state = STATE_ESCAPE;
                    }
                }
                break;
                case STATE_FIND_LAND:
                {

                }
                break;
                case STATE_ESCAPE:
                {
                    if(Bots.status[MyBotId] == BOT_NORMAL)
                        state = STATE_FIND_LAND;
                }
                break;
                case STATE_FIND_UNCROWDED:
                {
                    if(uncrowdedEnough(MyPosNow)){
                        state = STATE_FIND_LAND;
                        dbgprint(stderr, "%s\n", "in uncrowded area now");
                    }
                }
            }

            switch(state) {
                case STATE_ENCLOSING:
                {
                    int clk = ClockwiseTurn(lastDir);
                    int aclk = AntiClockwiseTurn(lastDir);
                    //优先选择右转,画螺旋形线
                    if(canGoInto(MyPosNow, clk) && !onTheTrack(getNextPoint(MyPosNow, clk), MyBotId)){
                        output_dir = clk;
                    }else if(canGoInto(MyPosNow, lastDir)) {
                        output_dir = lastDir;
                    }else if(canGoInto(MyPosNow, clk)) {
                        //不得不闭合轨迹了
                        output_dir = clk;
                    }else{
                        output_dir = aclk;
                    }

                    Point_t next = getNextPoint(MyPosNow, output_dir);
                    if(output_dir == lastDir){
                        //下一步所在的点不能一步收回
                        if(!canGoInto(next, clk) || !onTheTrack(getNextPoint(next, clk), MyBotId)){
                            if(NearestEnemyDist(Bots, getTrack(MyBotId)) <= 2){
                                if(canGoInto(MyPosNow, clk)) {
                                    //不得不闭合轨迹了
                                    dbgprint(stderr, "Don't be silly\n");
                                    output_dir = clk;
                                    break;
                                }
                            }
                        }
                    }

                    int ret = canAttack();
                    if(ret != -1)
                        output_dir = ret;
                }
                break;

                case STATE_FIND_LAND:
                {
                    std::pair<Point_t,int> ret = find_best_blank(distance);
                    Point_t &p = ret.first;
                    int &corner = ret.second;
                    if(p == MyPosNow){
                        int nearest = 0x2f2f2f2f;
                        for(int i=0; i<NUM_PLAYERS; i++){
                            if(i==MyBotId || Bots.status[i]==BOT_DEAD)
                                continue;
                            smaller_and_update(nearest, MyPosNow.dist(Bots.pos[i]));
                        }
                        if(nearest < 4){
                            state = STATE_FIND_LAND;
                            break;
                        }
                        //根据起点位置确定顺时针转的起始方向
                        switch(corner){
                            case UL_CORNER:
                                output_dir = DIR_RIGHT;
                                break;
                            case UR_CORNER:
                                output_dir = DIR_DOWN;
                                break;
                            case LL_CORNER:
                                output_dir = DIR_UP;
                                break;
                            case LR_CORNER:
                                output_dir = DIR_LEFT;
                                break;
                        }
                        output_action = ACTION_DRAW;
                        state = STATE_ENCLOSING;
                        startPoint = p;
                    }else{
                        output_dir = calc_next_step(p, distance, MyBotId);
                        int ret = canAttack();
                        if(ret != -1)
                            output_dir = ret;
                    }
                }
                break;

                case STATE_ESCAPE:
                {
                    int esc_dist[MAP_WIDTH+1][MAP_HEIGHT+1];
                    Point_t dest = chooseEscDest(esc_dist);
                    dbgprint(stderr, "escaping dest: (%d,%d)\n", dest.x, dest.y);
                    output_dir = calc_next_step(dest, esc_dist, LAND_NO_OWNER, lastPoint);
                }
                break;

                case STATE_FIND_UNCROWDED:
                {
                    Point_t tmp;
                    getUncrowded(tmp, distance);
                    output_dir = calc_next_step(tmp, distance, MyBotId);
                    int ret = canAttack();
                    if(ret != -1)
                        output_dir = ret;
                    // dbgprint(stderr, "%s %d\n", "STATE_FIND_UNCROWDED", output_dir);
                }
                break;
            }
            if(output_dir != DIR_STOP){
                lastPoint = MyPosNow;
                lastDir = output_dir;
            }
        }

        // debug_print_land();
        std::cout << "[ACTION] " << output_dir << ' ' << output_action << std::endl;
        debug_print_time_usage();
    }

    dbgprint(stderr, "%s\n", "debug: exit");

    return 0;
}