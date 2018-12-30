
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "curses.h"


typedef uint32_t b32;
#define False 0
#define True 1

#define ELF 'E' 
#define GOBLIN 'G'
#define OPEN '.'
#define WALL '#'

#define DELAY 300

#define LEN(a) (sizeof(a)/sizeof(a[0]))
#define ABS(a) ((a < 0) ? (-a) : (a))
#define MAX(a,b) ((a<b) ? (b) : (a))

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
    int power;
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
    int goblin_count;
    int elf_count;
    Entity** fighters;

    int queue_len;
    GridPoint* queue;
    int came_from_len;
    GridPoint* came_from;

} Map;
static Map map;

#define IDX(x,y) ((y)*map.xdim + (x)) 
#define X(idx) ((idx) % map.ydim) 
#define Y(idx) ((idx) / map.ydim) 

void bfs_attack(Entity* entity)
{
    GridPoint cur = {entity->x, entity->y, entity->hp, entity->type};
    GridPoint neighbors[4] = { 
        {cur.x, cur.y - 1, cur.val + 1, map.rows[cur.y - 1][cur.x]}, 
        {cur.x - 1, cur.y, cur.val + 1, map.rows[cur.y][cur.x - 1]}, 
        {cur.x + 1, cur.y, cur.val + 1, map.rows[cur.y][cur.x + 1]}, 
        {cur.x, cur.y + 1, cur.val + 1, map.rows[cur.y + 1][cur.x]}}; 
    Entity* in_range_enemies[4];
    int no_in_range = 0;
    for (int neighbor_idx = 0; neighbor_idx < LEN(neighbors); ++neighbor_idx)
    {
        GridPoint n = neighbors[neighbor_idx];
        switch (n.tile)
        {
            case ELF:
            case GOBLIN:
                {
                    if (n.tile != entity->type) // enemy 
                    {
                        for (int target_idx = 0; 
                                target_idx < (map.elf_count + map.goblin_count);
                                ++target_idx)
                        {
                            Entity* target = map.fighters[target_idx];
                            if (target->x == n.x && target->y == n.y)
                            {
                                in_range_enemies[no_in_range++] = target;
                            }
                        }
                    }
                } break;
        }
    }
    if (no_in_range > 0)
    {
        Entity* target = in_range_enemies[0]; 
        for (int target_idx = 1; 
                target_idx < no_in_range;
                ++target_idx)
        {
            if (in_range_enemies[target_idx]->hp < target->hp)
            {
                target = in_range_enemies[target_idx];
            }
        }
        target->hp -= entity->power;
        if (target->hp <= 0)
        {
            map.rows[target->y][target->x] = OPEN;
            target->y = map.ydim + 1; // for sorting
            if (target->type == ELF)
            {
                --map.elf_count;
            } else {
                --map.goblin_count;
            }
        }
    }
}


void bfs_move(Entity* entity)
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
    map.came_from[eidx].val = 0; // mark as visited
    GridPoint* target = NULL;
    GridPoint _target = {map.xdim, map.ydim, map.xdim*map.ydim};

    while (current_idx < map.queue_len)
    {
        GridPoint cur = map.queue[current_idx];
        GridPoint neighbors[4] = { 
            {cur.x, cur.y - 1, cur.val + 1, map.rows[cur.y - 1][cur.x]}, 
            {cur.x - 1, cur.y, cur.val + 1, map.rows[cur.y][cur.x - 1]}, 
            {cur.x + 1, cur.y, cur.val + 1, map.rows[cur.y][cur.x + 1]}, 
            {cur.x, cur.y + 1, cur.val + 1, map.rows[cur.y + 1][cur.x]}}; 
        int no_in_range = 0;
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
                case GOBLIN:
                {
                    if (n.tile != entity->type) // enemy 
                    {
                        if (target == NULL)
                        {
                            target = &_target;
                        }
                        if ((cur.val < target->val) || 
                            (cur.val == target->val && (IDX(cur.x, cur.x) < IDX(target->x, target->y))))
                        {
                            target->x = cur.x;
                            target->y = cur.y;
                            target->val = cur.val;
                        }
                    }
                } break;
            }
        }
        current_idx++;
    }

    if (target != NULL)
    {
        int gp_idx = IDX(target->x, target->y);
        int prev_gp_idx = gp_idx;
        GridPoint gp;
        while (gp_idx != eidx) {
            prev_gp_idx = gp_idx;
            gp = map.came_from[gp_idx]; 
            gp_idx = IDX(gp.x, gp.y);
        }
        int px = X(prev_gp_idx);
        int py = Y(prev_gp_idx);
        if (target->val > 0)
        {
            map.rows[entity->y][entity->x] = OPEN;
            map.rows[py][px] = entity->type;
            entity->x = px;
            entity->y = py;
        }
    }
}


void print_usage(char** argv)
{
    printf("Usage %s [-s|-p] <file.map>\n", argv[0]);
    printf("    -s: Step animation\n");
//    printf("    -p: solve part2\n");
}

void heapsort(Entity** heap, int heap_len);
void read_map_file(const char* path);
int main(int argc, char** argv)
{
    b32 step = False;
    b32 solve_part2 = False;
    if (argc < 2)
    {
        print_usage(argv);
        exit(1);
    }

    if (argc == 2)
    {
        read_map_file(argv[1]);
    } else if (argc > 2)
    {
        if (argv[1][0] == '-')
        {
            switch ( argv[1][1] )
            {
                case 's':
                    step = True;
                    break;

#if 0
                case 'p':
                    solve_part2 = True;
                    break;
#endif
            }
        }
        else
        {
            print_usage(argv); 
            exit(1);
        }

        read_map_file(argv[2]);
    }

    initscr();			/* Start curses mode 		  */
    cbreak();
    int rows, cols;
    heapsort(map.fighters, map.entity_count);
    int tempbuf_size = map.entity_count*8+1;
    char* temp_buf = malloc(sizeof(char)*tempbuf_size);
    memset(temp_buf, 0, tempbuf_size);
    for (int row_idx = 0; row_idx < map.row_count; ++row_idx)
    {
        mvprintw(row_idx, 0, "%s", map.rows[row_idx]);	/* Print line*/
    }
    mvprintw(map.ydim, 0, "Goblins: %d", map.goblin_count);
    mvprintw(map.ydim+1, 0, "Elves: %d", map.elf_count);
	refresh();			/* Print it on to the real screen */
	int ch = getch();			/* Wait for user input */

    int cont = 1;
    int finished_rounds = 0;
    while(cont)
    {
        for (int fidx = 0; fidx < map.entity_count; ++fidx)
        {
            Entity* current = map.fighters[fidx];
            if (current->hp > 0)
            {
                bfs_move(current);
                bfs_attack(current);
            }
        }
        heapsort(map.fighters, map.entity_count);
        int eidx = 0;
        for (int row_idx = 0; row_idx < map.row_count; ++row_idx)
        {
            int bufcur = 0;
            mvprintw(row_idx, 0, "%s", map.rows[row_idx]);
            for(; eidx < map.entity_count; ++eidx)
            {
                Entity* current = map.fighters[eidx];
                if (current->y == row_idx)
                {
                    bufcur += sprintf(temp_buf+bufcur, "%c(%3d) ", 
                            (current->type == ELF) ? 'E' : 'G', 
                            current->hp);
                }
                else
                {
                    break;
                }
            }
            temp_buf[bufcur] = ' ';
            mvprintw(row_idx, map.xdim + 1, "%s", temp_buf);
            memset(temp_buf, ' ', tempbuf_size);
            temp_buf[tempbuf_size-1] = '\0';
        }
        mvprintw(map.ydim, 0, "Goblins: %d", map.goblin_count);
        mvprintw(map.ydim+1, 0, "Elves: %d", map.elf_count);
        mvprintw(map.ydim+2, 0, "Round: %d", finished_rounds);
        refresh();			/* Print it on to the real screen */
        if (step)
        {
            ch = getch();			/* Wait for user input */
            if (ch == 'q')
            {
                cont = 0;
            }
        } 
        else 
        {
            Sleep(DELAY);
        }
        if (map.goblin_count == 0 || map.elf_count == 0)
        {
            cont = 0;
        }
        else
        {
            finished_rounds++;
        }
    }

    int remaining_hp = 0;
    for (int fidx = 0; fidx < map.entity_count; ++fidx)
    {
        Entity* current = map.fighters[fidx];
        if (current->hp > 0)
        {
            remaining_hp += current->hp;
        }
    }
    mvprintw(map.ydim+3, 0, "%s wins, score %d", 
             (map.goblin_count > map.elf_count) ? "Goblins" : "Elves",
             finished_rounds*remaining_hp);
    getch();
	endwin();			/* End curses mode		  */ 
    return 0;
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
    int goblin_count = 0;
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
        } else if (map.mem[idx] == GOBLIN)
        {
            goblin_count++;
        } else if (map.mem[idx] == ELF)
        {
            elf_count++;
        }
    }
    map.ydim = linecount;
    map.row_count = linecount;
    map.rows= malloc(sizeof(char*)*linecount);
    map.goblin_count = goblin_count;
    map.elf_count = elf_count;
    map.entity_count = goblin_count + elf_count;
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
            case GOBLIN:
                next_entity->x = ++x;
                next_entity->y = y;
                if (cur == ELF)
                {
                    next_entity->type = ELF;
                    next_entity->hp = 200;
                    next_entity->power = 3;
                } else {
                    next_entity->type = GOBLIN;
                    next_entity->hp = 200;
                    next_entity->power = 3;
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

void max_heapify(Entity** heap, int heap_size, int idx)
{
    Entity* cur = heap[idx];
    int cur_val = IDX(cur->x, cur->y);
    int left_idx = heap_left(idx);
    int right_idx = heap_right(idx);
    int largest_idx = idx;
    Entity* largest = cur;
    if (left_idx < heap_size)
    {
        Entity* left = heap[left_idx];
        int left_val = IDX(left->x, left->y);
        if (left_val > cur_val)
        {
            largest_idx = left_idx;
            largest = left;
        }
    }
    if (right_idx < heap_size)
    {
        Entity* right = heap[right_idx];
        int right_val = IDX(right->x, right->y);
        if (right_val > IDX(largest->x, largest->y))
        {
            largest_idx = right_idx;
            largest = right;
        }
    }
    if (largest_idx != idx)
    {
        Entity* temp = largest;
        heap[largest_idx] = heap[idx];
        heap[idx] = largest;
        max_heapify(heap, heap_size, largest_idx);
    }
}

void build_max_heap(Entity** heap, int heap_len)
{
    for (int i = heap_len / 2; i >= 0; --i)
    {
        max_heapify(heap, heap_len, i);
    }
}

void heapsort(Entity** heap, int heap_len)
{
    build_max_heap(heap, heap_len);
    int heap_size = heap_len;
    for (int i = heap_len-1; i > 0; --i)
    {
        Entity* temp = heap[i];
        heap[i] = heap[0];
        heap[0] = temp;
        heap_size--;
        max_heapify(heap, heap_size, 0);
    }
}
