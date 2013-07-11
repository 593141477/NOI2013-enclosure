#include <algorithm>
#include <vector>

#define MAP_WIDTH    10
#define MAP_HEIGHT   10
#define MAX_VERTICES ((MAP_WIDTH+1)*(MAP_HEIGHT+1))
#define NUM_PLAYERS   4
#define LAND_NO_OWNER -1

enum { BOT_DEAD=-1, BOT_NORMAL=0, BOT_DRAWING=1 };// 死亡,抬笔,落笔
enum { ACTION_NOTHING=0, ACTION_DRAW=1, ACTION_SKILL=-1};
enum { DIR_UP='u', DIR_DOWN='d', DIR_LEFT='l', DIR_RIGHT='r', DIR_STOP='s'};
enum { UL_CORNER, UR_CORNER, LL_CORNER, LR_CORNER};

const char DELTA_NAME[4]={DIR_DOWN, DIR_UP, DIR_RIGHT, DIR_LEFT};
const int DELTA[4][2]={{0,1},{0,-1},{1,0},{-1,0}};
const int DELTA_INVERSE[4]={1, 0, 3, 2};

struct Point_t
{
	int x,y;
	Point_t(){}
	Point_t(int _x, int _y):x(_x), y(_y){}
	bool operator==(const Point_t& t) const{
		return x==t.x && y==t.y;
	}
	bool equals(const int _x, const int _y) const{
		return x==_x && y==_y;
	}
	bool OutOfBounds() const{
		return x<0 || y<0 || x>MAP_WIDTH || y>MAP_HEIGHT;
	}
};
struct BotsInfo_t
{
	Point_t pos[NUM_PLAYERS];
	Point_t last_pos[NUM_PLAYERS];
	int status[NUM_PLAYERS];
	int trapped[NUM_PLAYERS];
	int scoredecline[NUM_PLAYERS];
};
struct Trap_t
{
	int x,y,left;
};

const BotsInfo_t& get_bots_info();

void init_map();
void read_Traps();
bool onTheTrack(const Point_t &p, int who);
const std::vector<Point_t>& getTrack(int who);
void update_map(const BotsInfo_t &);
void BfsSearch(int x, int y, int player, int distance[MAP_WIDTH+1][MAP_HEIGHT+1], Point_t lastPt = Point_t(-1,-1));
int getEdgeOwner(const Point_t &st, const Point_t &ed);
bool inline smaller_and_update(int &a, const int b);
std::pair<Point_t,int> find_nearest_blank(const int distance[MAP_WIDTH+1][MAP_HEIGHT+1]);
void debug_print_land();

int calc_next_step(const Point_t &dest, int distance[MAP_WIDTH+1][MAP_HEIGHT+1], int player, Point_t lastPt = Point_t(-1,-1));
int ClockwiseTurn(int dir);
int AntiClockwiseTurn(int dir);
Point_t getNextPoint(const Point_t &st, int Dir);
bool canGoInto(const Point_t &pos, int Dir);
bool inDangerNow(int me, const Point_t &start);
