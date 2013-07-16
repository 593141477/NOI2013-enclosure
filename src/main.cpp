#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctime>

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
    int lastDir, step_count=0;

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
                    step_count++;
                    int clk = ClockwiseTurn(lastDir);
                    int aclk = AntiClockwiseTurn(lastDir);
                    if(canGoInto(MyPosNow, lastDir)) {
                        output_dir = lastDir;
                    }else if(canGoInto(MyPosNow, clk)){
                        output_dir = clk;
                    }else{
                        output_dir = aclk;
                    }
                    if(step_count > 2){
                        if(canGoInto(MyPosNow, clk))
                            output_dir = clk;
                        else if(canGoInto(MyPosNow, aclk))
                            output_dir = aclk;
                    }
                    if(output_dir != lastDir)
                        step_count = 0;
                    if(onTheTrack(getNextPoint(MyPosNow, output_dir), MyBotId)) {
                        state = STATE_FIND_LAND;
                    }else if(inDangerNow(startPoint)){
                        dbgprint(stderr, "%s\n", "start escaping...");
                        state = STATE_ESCAPE;
                    }else{
                        //attack
                        for (int i = 0; i < NUM_PLAYERS; ++i){
                            if(i==MyBotId || Bots.status[i]==BOT_DEAD)
                                continue;
                            for(int j=0; j<4; j++){
                                Point_t target(MyPosNow.x+DELTA[j][0], MyPosNow.y+DELTA[j][1]);
                                if(!target.OutOfBounds() && onTheTrack(target, i) && canGoInto(MyPosNow, DELTA_NAME[j])){
                                    output_dir = DELTA_NAME[j];
                                    dbgprint(stderr, "%s\n", "attack");
                                    goto attack;
                                }
                            }
                        }
                    attack:;
                    }
                }
                break;

                case STATE_FIND_LAND:
                {
                    int corner, blank_size;
                    Point_t p = find_nearest_blank(distance, corner, blank_size);
                    if(blank_size < 4){
                        int largest = getLargestBlankSize();
                        p = find_nearest_blank(distance, corner, blank_size, largest);
                    }
                    if(p == MyPosNow){
                        int nearest = 0x2f2f2f2f;
                        for(int i=0; i<NUM_PLAYERS; i++){
                            if(i==MyBotId || Bots.status[i]==BOT_DEAD)
                                continue;
                            smaller_and_update(nearest, MyPosNow.dist(Bots.pos[i]));
                        }
                        if(nearest < 4){
                            state = STATE_FIND_UNCROWDED;
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
                        step_count = 0;
                        startPoint = p;
                    }else{
                        output_dir = calc_next_step(p, distance, MyBotId);
                    }
                }
                break;

                case STATE_ESCAPE:
                {
                    int esc_dist[MAP_WIDTH+1][MAP_HEIGHT+1];
                    Point_t dest = chooseEscDest(esc_dist);
                    dbgprint(stderr, "escaping dest: (%d,%d)\n", dest.x, dest.y);
                    output_dir = calc_next_step(dest, esc_dist, LAND_NO_OWNER, lastPoint);

                    if(onTheTrack(getNextPoint(MyPosNow, output_dir), MyBotId)) 
                        state = STATE_FIND_LAND;
                }
                break;

                case STATE_FIND_UNCROWDED:
                {
                    Point_t tmp;
                    getUncrowded(tmp, distance);
                    output_dir = calc_next_step(tmp, distance, MyBotId);
                    // dbgprint(stderr, "%s %d\n", "STATE_FIND_UNCROWDED", output_dir);
                    if(uncrowdedEnough(getNextPoint(MyPosNow, output_dir))){
                        state = STATE_FIND_LAND;
                        dbgprint(stderr, "%s\n", "in uncrowded area now");
                    }
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