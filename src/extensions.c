#include "extensions.h"
#include "options.h"

//////////////////////////////////////////////////////////////////////
// For sorting colors

int color_features_compare(const void* vptr_a, const void* vptr_b) {

	const color_features_t* a = (const color_features_t*)vptr_a;
	const color_features_t* b = (const color_features_t*)vptr_b;

	int u = cmp(a->user_index, b->user_index);
	if (u) { return u; }

	int w = cmp(a->wall_dist[0], b->wall_dist[0]);
	if (w) { return w; }

	int g = -cmp(a->wall_dist[1], b->wall_dist[1]);
	if (g) { return g; }

	return -cmp(a->min_dist, b->min_dist);

}

//////////////////////////////////////////////////////////////////////
// Place the game colors into a set order

void game_order_colors(game_info_t* info,
                       game_state_t* state) {

	if (g_options.order_random) {
    
		srand(now() * 1e6);
    
		for (size_t i=info->num_colors-1; i>0; --i) {
			size_t j = rand() % (i+1);
			int tmp = info->color_order[i];
			info->color_order[i] = info->color_order[j];
			info->color_order[j] = tmp;
		}

	} else { // not random

		color_features_t cf[MAX_COLORS];
		memset(cf, 0, sizeof(cf));

		for (size_t color=0; color<info->num_colors; ++color) {
			cf[color].index = color;
			cf[color].user_index = MAX_COLORS;
		}
    

		for (size_t color=0; color<info->num_colors; ++color) {
			
			int x[2], y[2];
			
			for (int i=0; i<2; ++i) {
				pos_get_coords(state->pos[color], x+i, y+i);
				cf[color].wall_dist[i] = get_wall_dist(info, x[i], y[i]);
			}

			int dx = abs(x[1]-x[0]);
			int dy = abs(y[1]-y[0]);
			
			cf[color].min_dist = dx + dy;



		}


		qsort(cf, info->num_colors, sizeof(color_features_t),
		      color_features_compare);

		for (size_t i=0; i<info->num_colors; ++i) {
			info->color_order[i] = cf[i].index;
		}
    
	}

	if (!g_options.display_quiet) {

		printf("\n************************************************"
		       "\n*               Branching Order                *\n");
		if (g_options.order_most_constrained) {
			printf("* Will choose color by most constrained\n");
		} else {
			printf("* Will choose colors in order: ");
			for (size_t i=0; i<info->num_colors; ++i) {
				int color = info->color_order[i];
				printf("%s", color_name_str(info, color));
			}
			printf("\n");
		}
		printf ("*************************************************\n\n");

	}

}

//////////////////////////////////////////////////////////////////////
// This method checks if the given pos is a deadend by checking its 
// surroundings and calculate the free spaces around it. If the number
// of free spaces is less than 2, that means the pos is a
// deadend.

int pos_is_deadend(const game_info_t *info, const game_state_t *state, pos_t pos) {
	
	// if the pos isn't in bound, skip this check
	if (pos == INVALID_POS) {

		return 0;

	}

	int x, y;
	pos_get_coords(pos, &x, &y);
	
	// pos is a free cell
	if (game_is_free(info, state, x, y)) {

		// start checking if this free cell is a dead end
		int free_space_around = 0;

		for (int dir = DIR_LEFT; dir <= DIR_DOWN; dir ++) {

			pos_t adj_pos = pos_offset_pos(info, pos, dir);

			if (adj_pos == INVALID_POS) {

				continue;

			}

			// check for free space condition
			cell_t cell = state->cells[adj_pos];

			// the cell is occupied
			if (cell) {

				// get the color of the cell
				int color = cell_get_color(cell);

				// if the color path is incompleted and (adj is the end of the progress path || adj is the goal of the color path)
				if ((adj_pos == info->goal_pos[color] || adj_pos == state->pos[color]) && !(state->completed & (1 << color))) {
					
					free_space_around ++;
				
				}

			// the cell isn't occupied
			} else {
				
				free_space_around ++;
			
			}

		}

		if (free_space_around < 2) {

			/* deadend detected, stop searching continue */
			return 1;

		}

	}

	// not a deadend, keep searching
	return 0;

}

//////////////////////////////////////////////////////////////////////
// Check for dead-end regions of freespace where there is no way to
// put an active path into and out of it. Any freespace node which
// has only one free neighbor represents such a dead end. For the
// purposes of this check, cur and goal positions count as "free".

int game_check_deadends(const game_info_t* info,
                        const game_state_t* state) {
	/**
	 * FILL CODE TO DETECT DEAD-ENDS
	*/
	size_t last_color = state->last_color;

	pos_t head_pos = state->pos[last_color];

	// free space conditions: {the end of a progress path || free space || not a goal state}
	// perform 12 cells check
	// start by check the four adjcent cells around the 'head_pos' 
	for (int dir_a = DIR_LEFT; dir_a <= DIR_DOWN; dir_a ++) {

		pos_t adj_pos = pos_offset_pos(info, head_pos, dir_a);
	
		// check if it is a deadend
		if (pos_is_deadend(info, state, adj_pos) == 1) {

			return 1;

		}

		// check the four adjcent cells around each 'adj_pos'
		for (int dir_b = DIR_LEFT; dir_b <= DIR_DOWN; dir_b ++) {

			pos_t adj_adj_pos = pos_offset_pos(info, adj_pos, dir_b);

			// check if it is a deadend 
			if (pos_is_deadend(info, state, adj_adj_pos) == 1) {

				return 1;
			
			}	

		}

	}	

	// no deadend detected in this state, return 0
	return 0;
	
} 