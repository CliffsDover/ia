#ifndef MARKER_HPP
#define MARKER_HPP

#include <functional>
#include <climits>

#include "rl_utils.hpp"
#include "global.hpp"

struct Key_data;
struct Cell_overlay;

enum class Marker_done              {no, yes};
enum class Marker_use_player_tgt    {no, yes};

//Used for specifying if marker should be drawn red when aiming past
//seen cells blocking projectiles
enum class Marker_show_blocked      {no, yes};

namespace marker
{

P run(const Marker_use_player_tgt use_tgt,
      std::function<void(const P&,
                         Cell_overlay overlay[map_w][map_h])> on_marker_at_pos,
      std::function<Marker_done(const P&,
                                const Key_data&)> on_key_press,
      Marker_show_blocked show_blocked,
      const int EFFECTIVE_RANGE_LMT = INT_MAX);

} //marker

#endif
