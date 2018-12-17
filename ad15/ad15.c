
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "curses.h"

#define ELF 'E' 
#define GHOUL 'G'
#define OPEN '.'
#define WALL '#'

#define LEN(a) (sizeof(a)/sizeof(a[0]))

typedef struct
{
    int x;
    int y;
    int val;
    char tile;
} GridPoint;

typedef struct
{
    int x;
    int y;
    int hp;
    int type;
} Entity;

typedef struct 
{
    int xdim;
    int ydim;
    long size;
    char* mem;
    int row_count;
    char** rows;
    int entity_count;
    Entity* entities;
    int ghoul_count;
    int elf_count;
    Entity** fighters;

    int queue_len;
    GridPoint* queue;
    int came_from_len;
    GridPoint* came_from;

} Map;
static Map map;

inline int heap_parent(idx)
{
    return idx / 2;
}

inline int heap_left(idx)
{
    return 2*idx;
}

inline int heap_right(idx)
{
    return 2*idx + 1;
}

#define IDX(x,y) ((y)*map.xdim + (x)) 
void bfs_distance(Entity* entity)
{
    map.queue_len = 0;
    memset(map.queue, 0, sizeof(GridPoint)*map.xdim*map.ydim);
    map.came_from_len = 0;
    memset(map.came_from, -1, sizeof(GridPoint)*map.xdim*map.ydim);

    int current_idx = 0;
    map.queue[current_idx].x = entity->x;
    map.queue[current_idx].y = entity->y;
    map.queue[current_idx].val = 0;
    map.queue[current_idx].tile = OPEN;
    map.queue_len++;

    map.came_from[IDX(entity->x, entity->y)].x = entity->x;
    map.came_from[IDX(entity->x, entity->y)].y = entity->y;
    map.came_from[IDX(entity->x, entity->y)].val = 0;

    while (current_idx < map.queue_len)
    {
        GridPoint cur = map.queue[current_idx];
        GridPoint neighbors[4] = { 
            {cur.x, cur.y - 1, cur.val + 1, map.rows[cur.y - 1][cur.x]}, 
            {cur.x - 1, cur.y, cur.val + 1, map.rows[cur.y][cur.x - 1]}, 
            {cur.x + 1, cur.y, cur.val + 1, map.rows[cur.y][cur.x + 1]}, 
            {cur.x, cur.y + 1, cur.val + 1, map.rows[cur.y + 1][cur.x]}}; 
        for (int neighbor_idx = 0; neighbor_idx < LEN(neighbors); ++neighbor_idx)
        {
            if (neighbors[neighbor_idx].tile == OPEN)
            {
                int x = neighbors[neighbor_idx].x;
                int y = neighbors[neighbor_idx].y;
                if (map.came_from[IDX(x,y)].val < 0)  //not visited
                {
                    map.queue[map.queue_len++] = neighbors[neighbor_idx];
                    map.came_from[IDX(x,y)] = cur;
                }
            }
        }
        current_idx++;
    }
}

void bfs_action(Entity* entity)
{
    map.queue_len = 0;
    memset(map.queue, 0, sizeof(GridPoint)*map.xdim*map.ydim);
    map.came_from_len = 0;
    memset(map.came_from, -1, sizeof(GridPoint)*map.xdim*map.ydim);

    int current_idx = 0;
    map.queue[current_idx].x = entity->x;
    map.queue[current_idx].y = entity->y;
    map.queue[current_idx].val = 0;
    map.queue[current_idx].tile = OPEN;
    map.queue_len++;

    int eidx = IDX(entity->x, entity->y);
    map.came_from[eidx].x = entity->x;
    map.came_from[eidx].y = entity->y;
    map.came_from[eidx].val = 0;

    int found = 0;
    while (current_idx < map.queue_len && !found)
    {
        GridPoint cur = map.queue[current_idx];
        GridPoint neighbors[4] = { 
            {cur.x, cur.y - 1, cur.val + 1, map.rows[cur.y - 1][cur.x]}, 
            {cur.x - 1, cur.y, cur.val + 1, map.rows[cur.y][cur.x - 1]}, 
            {cur.x + 1, cur.y, cur.val + 1, map.rows[cur.y][cur.x + 1]}, 
            {cur.x, cur.y + 1, cur.val + 1, map.rows[cur.y + 1][cur.x]}}; 
        for (int neighbor_idx = 0; neighbor_idx < LEN(neighbors); ++neighbor_idx)
        {
            GridPoint n = neighbors[neighbor_idx];
            switch (n.tile)
            {
                case OPEN:
                {
                    int x = n.x;
                    int y = n.y;
                    if (map.came_from[IDX(x,y)].val < 0)  //not visited
                    {
                        map.queue[map.queue_len++] = n;
                        map.came_from[IDX(x,y)] = cur;
                    }
                } break;

                case ELF:
                case GHOUL:
                {
                    if (n.tile != entity->type) // enemy 
                    {
                        int gp_idx = IDX(cur.x, cur.y);
                        GridPoint gp;
                        while (gp_idx != eidx) {
                            gp = map.came_from[gp_idx]; 
                            gp_idx = IDX(gp.x, gp.y);
                        }
                        if (gp.val > 1)
                        {
                            map.rows[entity->y][entity->x] = OPEN;
                            map.rows[gp.y][gp.x] = entity->type;
                            entity->x = gp.x;
                            entity->y = gp.y;
                            found = 1;
                        }
                    }
                } break;
            }
        }
        current_idx++;
    }
}



void read_map_file(const char* path)
{
    FILE* file = fopen(path, "r");
    if (!file)
    {
        fprintf(stderr, "Could open file %s.\n", path);
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    long mapsize = ftell(file);
    if (mapsize <= 0)
    {
        fprintf(stderr, "Could not find end of file %s.\n", path);
        exit(1);
    }
    fseek(file, 0, SEEK_SET);
    map.mem = malloc(sizeof(char)*(mapsize+1));
    if (map.mem == NULL)
    {
        fprintf(stderr, "Could allocate memory for file %s.\n", path);
        exit(1);
    }
    fread(map.mem, mapsize, sizeof(char), file);
    fclose(file);
    map.mem[mapsize] = '\0';
    map.size = mapsize;

    long linecount = 0;
    long xdim = 0;
    int ghoul_count = 0;
    int elf_count = 0;
    for (long idx = 0; idx < map.size; idx++, xdim++)
    {
        if(map.mem[idx] == '\n')
        {
            if (idx > 0 && map.mem[idx -1] == '\r')
            {
                --xdim;
                map.mem[idx -1] = '\0';
            }
            map.mem[idx] = '\0';
            map.xdim = xdim - 1;
            xdim = 0;
            linecount++;
        } else if (map.mem[idx] == GHOUL)
        {
            ghoul_count++;
        } else if (map.mem[idx] == ELF)
        {
            elf_count++;
        }
    }
    map.ydim = linecount;
    map.row_count = linecount;
    map.rows= malloc(sizeof(char*)*linecount);
    map.ghoul_count = ghoul_count;
    map.elf_count = elf_count;
    map.entity_count = ghoul_count + elf_count;
    map.entities = malloc(sizeof(Entity)*map.entity_count);
    map.fighters = malloc(sizeof(Entity*)*map.entity_count);
    map.queue_len = 0;
    map.queue = malloc(sizeof(GridPoint)*map.xdim*map.ydim);
    memset(map.queue, 0, sizeof(GridPoint)*map.xdim*map.ydim);
    map.came_from_len = 0;
    map.came_from = malloc(sizeof(GridPoint)*map.xdim*map.ydim);
    memset(map.came_from, -1, sizeof(GridPoint)*map.xdim*map.ydim);

    Entity* next_entity = map.entities;
    Entity** next_fighter = map.fighters;
    map.rows[0] = map.mem;
    int x = 0;
    int y = 0;
    for (int idx = 0; idx < map.size;)
    {
        char cur = map.mem[idx];
        switch(cur)
        {
            case ELF:
            case GHOUL:
                next_entity->x = ++x;
                next_entity->y = y;
                if (cur == ELF)
                {
                    next_entity->type = ELF;
                    next_entity->hp = 5;
                } else {
                    next_entity->type = GHOUL;
                    next_entity->hp = 5;
                }
                *next_fighter = next_entity;
                next_entity++;
                next_fighter++;
                //map.mem[idx] = '.';
                break;
            case '\0':
                x=0;
                while(map.mem[++idx] == '\0');
                map.rows[++y] = map.mem + idx;
                break;
            default:
                ++x;
        }
        ++idx;
    }
}

int main(int argc, char** argv)
{
    read_map_file("input_15.txt");

    initscr();			/* Start curses mode 		  */
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    //mvprintw(rows/2, 0, "%d, %d", rows, cols);
    for (int row_idx = 0; row_idx < map.row_count; ++row_idx)
    {
        mvprintw(row_idx, 0, "%s", map.rows[row_idx]);	/* Print line*/
    }
#if 0
    for (int idx = 0; idx < map.elf_count + map.ghoul_count; ++idx)
    {
        Entity* ent = map.fighters[idx];
        char* c = (map.fighters[idx]->type == ELF) ? "E" : "G";
        mvprintw(ent->y, map.fighters[idx]->x, c);
    }
#endif
    bfs_action(map.entities);
    mvprintw(map.ydim, 0, "Ghouls: %d", map.ghoul_count);
    mvprintw(map.ydim+1, 0, "Elves: %d", map.elf_count);
	refresh();			/* Print it on to the real screen */
	getch();			/* Wait for user input */
    for (int row_idx = 0; row_idx < map.row_count; ++row_idx)
    {
        mvprintw(row_idx, 0, "%s", map.rows[row_idx]);	/* Print line*/
    }
	refresh();			/* Print it on to the real screen */
	getch();			/* Wait for user input */
	endwin();			/* End curses mode		  */ 
    return 0;
}
