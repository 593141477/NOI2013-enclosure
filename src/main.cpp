#include <iostream>
#include <cstdio>
#include <cstdlib>

void init_program(int entropy)
{
    srand((time(0)^1995) + entropy);

    init_map();
}

void output_init_pos()
{
    std::cout << "[POS] " << rand()%(MAP_WIDTH+1) << ' ' << rand()%(MAP_HEIGHT+1) << std::endl;
}
void read_situation(BotsInfo_t &Bots)
{
    std::string tmp;
    std::cin >> tmp;
    for(int i=0; i<NUM_PLAYERS; i++) {
        Bots.last_pos[i] = Bots.pos[i];
        std::cin >> Bots.pos[i].x >> Bots.pos[i].y;
        std::cin >> Bots.status[i] >> Bots.trapped[i] >> Bots.scoredecline[i];
    }
    read_Traps();
}
void debug_print_distance(int distance[MAP_WIDTH+1][MAP_HEIGHT+1])
{
    fprintf(stderr, "======distance======\n");
    for(int i=0; i<=MAP_HEIGHT; i++){
        for(int j=0; j<=MAP_WIDTH; j++){
            if(distance[j][i] == 0x2f2f2f2f)
                fprintf(stderr, "%s, ", "oo");
            else
                fprintf(stderr, "%02d, ", distance[j][i]);
        }
        fprintf(stderr, "\n");
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

    int state = STATE_FIND_LAND;

    std::cin >> tmp >> MyBotId;

    init_program(MyBotId);
    output_init_pos();

    Bots.MyID = MyBotId;

    for(Round=1; Round<=100; Round++) {
        char output_dir;
        int output_action;
        read_situation(Bots);
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
                    if(step_count > 3){
                        if(canGoInto(MyPosNow, clk))
                            output_dir = clk;
                        else if(canGoInto(MyPosNow, aclk))
                            output_dir = aclk;
                    }
                    if(output_dir != lastDir)
                        step_count = 0;
                    if(onTheTrack(getNextPoint(MyPosNow, output_dir), MyBotId)) {
                        state = STATE_FIND_LAND;
                    }else if(inDangerNow(MyBotId, startPoint)){
                        fprintf(stderr, "%s\n", "start escaping...");
                        state = STATE_ESCAPE;
                    }
                }
                break;

                case STATE_FIND_LAND:
                {
                    std::pair<Point_t,int> Pair;
                    Pair = find_nearest_blank(distance);
                    Point_t p = Pair.first;
                    if(p == MyPosNow){
                        //根据起点位置确定顺时针转的起始方向
                        switch(Pair.second){
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
                    Point_t dest = chooseEscDest(esc_dist, MyBotId);
                    fprintf(stderr, "escaping dest: (%d,%d)\n", dest.x, dest.y);
                    output_dir = calc_next_step(dest, esc_dist, MyBotId, lastPoint);

                    if(onTheTrack(getNextPoint(MyPosNow, output_dir), MyBotId)) 
                        state = STATE_FIND_LAND;
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
    }

    return 0;
}