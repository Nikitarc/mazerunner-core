/******************************************************************************
 * Project: mazerunner-core                                                   *
 * -----                                                                      *
 * Copyright 2022 - 2023 Peter Harrison, Micromouseonline                     *
 * -----                                                                      *
 * Licence:                                                                   *
 *     Use of this source code is governed by an MIT-style                    *
 *     license that can be found in the LICENSE file or at                    *
 *     https://opensource.org/licenses/MIT.                                   *
 ******************************************************************************/

#ifndef MAZE_H
#define MAZE_H

/***
 * The Maze class holds the map of the maze as the state of all four
 * walls in each cell.
 *
 * When looking for exits, you can set a view mask. The mask is:
 *      MASK_OPEN => unseen walls are treated as exits
 *    MASK_CLOSED => unseen walls are treated as walls
 *
 * Set the mask to OPEN while searching and CLOSED when calculating a speed run
 *
 * A cell is considered to have been visited if all of its walls have been seen.
 *
 * There are a number of supporting structures and types. These are described below.
 *
 */

#include <stdint.h>
#include "queue.h"

#define START Location(0, 0)

/***
 * Walls exist in the map in one of four states and so get recorded using
 * two bits in the map. There are various schemes for this but here the
 * state is held in a single entity for each wall. Symbolic names for each
 * state are listed in the WallState enum.
 *
 * Virtual walls are not used in this code but are labelled for completeness
 */
enum WallState {
  EXIT = 0,     // a wall that has been seen and confirmed absent
  WALL = 1,     // a wall that has been seen and confirmed present
  UNKNOWN = 2,  // a wall that has not yet been seen
  VIRTUAL = 3,  // a wall that has not yet been seen
};

//***************************************************************************//
/***
 * The state of all four walls in a cell are stored as a structure in a single
 * variable.
 *
 * Note that GCC for AVR and STM32 (and probably most other) targets should
 * recognise this as representing a single byte in memory but that is not
 * guaranteed.
 */
struct WallInfo {
  WallState north : 2;
  WallState east : 2;
  WallState south : 2;
  WallState west : 2;
};

/***
 * Since maze wall state can have one of four values, the MazeMask
 * is used to let you use or ignore the fact that a wall has been seen.
 *
 * When the mask is set to MASK_OPEN, any unseen walls are treated as
 * being absent. Setting it to MASK_CLOSED treats unseen walls as present.
 *
 * Practically, this means that fast runs should be calculated after flooding
 * the maze with the MASK_CLOSE option so that the route never passes through
 * unseen walls or cels.
 *
 * Searching should be performed with the MASK_OPEN option so that the flood
 * uses all the cels.
 */
enum MazeMask {
  MASK_OPEN = 0x01,    // open maze for search
  MASK_CLOSED = 0x03,  // closed maze for fast run
};

//***************************************************************************//

/***
 * A Heading represents one of the four cardinal compass headings. Normally
 * this is sufficient. If you are going to run diagonals, you might want to
 * expand the list.
 *
 * The compiler will number enums consecutively starting at zero so we can
 * use a couple of tricks to work out what the new value should be when
 * the robot turns.
 *
 * If you expand the list for diagonals, take care to modify the calculations
 * appropriately.
 */
enum Heading { NORTH, EAST, SOUTH, WEST, HEADING_COUNT, BLOCKED = 99 };

inline Heading right_from(const Heading heading) {
  return static_cast<Heading>((heading + 1) % HEADING_COUNT);
}

inline Heading left_from(const Heading heading) {
  return static_cast<Heading>((heading + HEADING_COUNT - 1) % HEADING_COUNT);
}

inline Heading ahead_from(const Heading heading) {
  return heading;
}

inline Heading behind_from(const Heading heading) {
  return static_cast<Heading>((heading + 2) % HEADING_COUNT);
}
//***************************************************************************//

enum Direction { AHEAD, RIGHT, BACK, LEFT, DIRECTION_COUNT };
//***************************************************************************//

#define MAX_COST 255
#define MAZE_WIDTH 16
#define MAZE_CELL_COUNT (MAZE_WIDTH * MAZE_WIDTH)

//***************************************************************************//

struct Location {
  uint8_t x;
  uint8_t y;

  Location(uint8_t ix = 0, uint8_t iy = 0) : x(ix), y(iy){};

  bool is_in_maze() {
    return x < MAZE_WIDTH && y < MAZE_WIDTH;
  }

  bool operator==(const Location &obj) const {
    return x == obj.x && y == obj.y;
  }

  bool operator!=(const Location &obj) const {
    return x != obj.x || y != obj.y;
  }

  // these operators prevent the user from exceeding the bounds of the maze
  // by wrapping to the opposite edge
  Location north() const {
    return Location(x, (y + 1) + MAZE_WIDTH);
  }

  Location east() const {
    return Location((x + 1) + MAZE_WIDTH, y);
  }

  Location south() const {
    return Location(x, (y + MAZE_WIDTH - 1) % MAZE_WIDTH);
  }

  Location west() const {
    return Location((x + MAZE_WIDTH - 1) % MAZE_WIDTH, y);
  }

  Location neighbour(const Heading heading) const {
    switch (heading) {
      case NORTH:
        return north();
        break;
      case EAST:
        return east();
        break;
      case SOUTH:
        return south();
        break;
      case WEST:
        return west();
        break;
      default:
        return *this;
        break;
    }
  }
};

//***************************************************************************//

class Maze {
 public:
  Maze() {
  }

  Location goal() const {
    return m_goal_loc;
  }

  void set_goal(const Location goal) {
    m_goal_loc = goal;
  }

  WallInfo walls(const Location loc) const {
    return m_walls[loc.x][loc.y];
  }

  bool has_unknown_walls(const Location cell) const {
    WallInfo walls_here = m_walls[cell.x][cell.y];
    if (walls_here.north == UNKNOWN || walls_here.east == UNKNOWN || walls_here.south == UNKNOWN || walls_here.west == UNKNOWN) {
      return true;
    } else {
      return false;
    }
  }

  bool cell_is_visited(const Location cell) const {
    return not has_unknown_walls(cell);
  }

  bool is_exit(const Location loc, const Heading heading) const {
    bool result = false;
    WallInfo walls = m_walls[loc.x][loc.y];
    switch (heading) {
      case NORTH:
        result = (walls.north & m_mask) == EXIT;
        break;
      case EAST:
        result = (walls.east & m_mask) == EXIT;
        break;
      case SOUTH:
        result = (walls.south & m_mask) == EXIT;
        break;
      case WEST:
        result = (walls.west & m_mask) == EXIT;
        break;
      default:
        result = false;
        break;
    }
    return result;
  }

  // Unconditionally set a wall state.
  // Normally you only use this in setting up the maze at the start
  void set_wall_state(const Location loc, const Heading heading, const WallState state) {
    switch (heading) {
      case NORTH:
        m_walls[loc.x][loc.y].north = state;
        m_walls[loc.north().x][loc.north().y].south = state;
        break;
      case EAST:
        m_walls[loc.x][loc.y].east = state;
        m_walls[loc.east().x][loc.east().y].west = state;
        break;
      case WEST:
        m_walls[loc.x][loc.y].west = state;
        m_walls[loc.west().x][loc.west().y].east = state;
        break;
      case SOUTH:
        m_walls[loc.x][loc.y].south = state;
        m_walls[loc.south().x][loc.south().y].north = state;
        break;
      default:
        // ignore any other heading (blocked)
        break;
    }
  }

  // only change a wall if it is unknown
  // This is what you use when exploring. Once seen, a wall should not be changed again.
  void update_wall_state(const Location loc, const Heading heading, const WallState state) {
    switch (heading) {
      case NORTH:
        if ((m_walls[loc.x][loc.y].north & UNKNOWN) != UNKNOWN) {
          return;
        }
        break;
      case EAST:
        if ((m_walls[loc.x][loc.y].east & UNKNOWN) != UNKNOWN) {
          return;
        }
        break;
      case WEST:
        if ((m_walls[loc.x][loc.y].west & UNKNOWN) != UNKNOWN) {
          return;
        }
        break;
      case SOUTH:
        if ((m_walls[loc.x][loc.y].south & UNKNOWN) != UNKNOWN) {
          return;
        }
        break;
      default:
        // ignore any other heading (blocked)
        break;
    }
    set_wall_state(loc, heading, state);
  }

  /***
   * Initialise a maze and the costs with border walls and the start cell
   *
   */
  void initialise() {
    for (int x = 0; x < MAZE_WIDTH; x++) {
      for (int y = 0; y < MAZE_WIDTH; y++) {
        m_walls[x][y].north = UNKNOWN;
        m_walls[x][y].east = UNKNOWN;
        m_walls[x][y].south = UNKNOWN;
        m_walls[x][y].west = UNKNOWN;
      }
    }
    for (int x = 0; x < MAZE_WIDTH; x++) {
      m_walls[x][0].south = WALL;
      m_walls[x][MAZE_WIDTH - 1].north = WALL;
    }
    for (int y = 0; y < MAZE_WIDTH; y++) {
      m_walls[0][y].west = WALL;
      m_walls[MAZE_WIDTH - 1][y].east = WALL;
    }
    m_walls[0][0].north = EXIT;
    m_walls[0][0].east = WALL;
    m_walls[0][0].south = WALL;
    m_walls[0][0].west = WALL;

    // the open maze treats unknowns as exits
    set_mask(MASK_OPEN);
  }

  void set_mask(const MazeMask mask) {
    m_mask = mask;
  }

  MazeMask get_mask() const {
    return m_mask;
  }

  /***
   * Assumes the maze has been flooded
   */

  uint16_t neighbour_cost(const Location cell, const Heading heading) const {
    if (not is_exit(cell, heading)) {
      return MAX_COST;
    }
    Location next_cell = cell.neighbour(heading);
    return m_cost[next_cell.x][next_cell.y];
  }

  uint16_t cost(const Location cell) const {
    return m_cost[cell.x][cell.y];
  }

  /***
   * Very simple cell counting flood fills m_cost array with the
   * manhattan distance from every cell to the target.
   *
   * Although the queue looks complicated, this is a fast flood that
   * examines each accessible cell exactly once. Consequently, it runs
   * in fairly constant time, taking 5.3ms when there are no interrupts.
   *
   * @param target - the cell from which all distances are calculated
   */

  void flood(const Location target) {
    for (int x = 0; x < MAZE_WIDTH; x++) {
      for (int y = 0; y < MAZE_WIDTH; y++) {
        m_cost[x][y] = (uint8_t)MAX_COST;
      }
    }
    Queue<Location, 64> queue;
    m_cost[target.x][target.y] = 0;
    queue.add(target);
    while (queue.size() > 0) {
      Location here = queue.head();
      uint16_t newCost = m_cost[here.x][here.y] + 1;
      // the casting of enums is potentially problematic
      for (int h = NORTH; h < HEADING_COUNT; h++) {
        Heading heading = static_cast<Heading>(h);
        if (is_exit(here, heading)) {
          Location nextCell = here.neighbour(heading);
          if (m_cost[nextCell.x][nextCell.y] > newCost) {
            m_cost[nextCell.x][nextCell.y] = newCost;
            queue.add(nextCell);
          }
        }
      }
    }
  }

  /***
   * Algorithm looks around the current cell and records the smallest
   * neighbour and its direction. By starting with the supplied direction,
   * then looking right, then left, the result will preferentially be
   * ahead if there are multiple neighbours with the same m_cost.
   *
   * This could be extended to look ahead then towards the goal but it
   * probably is not worth the effort
   *
   * @param cell
   * @param start_heading
   * @return
   */
  Heading heading_to_smallest(const Location cell, const Heading start_heading) const {
    Heading next_heading = start_heading;
    Heading best_heading = BLOCKED;
    uint16_t best_cost = cost(cell);
    uint16_t cost;
    cost = neighbour_cost(cell, next_heading);
    if (cost < best_cost) {
      best_cost = cost;
      best_heading = next_heading;
    };
    next_heading = right_from(start_heading);
    cost = neighbour_cost(cell, next_heading);
    if (cost < best_cost) {
      best_cost = cost;
      best_heading = next_heading;
    };
    next_heading = left_from(start_heading);
    cost = neighbour_cost(cell, next_heading);
    if (cost < best_cost) {
      best_cost = cost;
      best_heading = next_heading;
    };
    next_heading = behind_from(start_heading);
    cost = neighbour_cost(cell, next_heading);
    if (cost < best_cost) {
      best_cost = cost;
      best_heading = next_heading;
    };
    if (best_cost == MAX_COST) {
      best_heading = BLOCKED;
    }
    return best_heading;
  }

 private:
  MazeMask m_mask = MASK_OPEN;
  Location m_goal_loc{7, 7};
  // on Arduino only use 8 bits for cost to save space
  uint8_t m_cost[MAZE_WIDTH][MAZE_WIDTH];
  WallInfo m_walls[MAZE_WIDTH][MAZE_WIDTH];
};

extern Maze maze;

#endif  // MAZE_H
