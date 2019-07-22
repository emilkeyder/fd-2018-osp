Fast Downward is a domain-independent planning system.

For documentation and contact information see http://www.fast-downward.org/.

The following directories are not part of Fast Downward as covered by this
license:

* ./src/search/ext

For the rest, the following license applies:

```
Fast Downward is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

Fast Downward is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.
```

The A* OSP planner introduced in the following paper (Katz & Keyder, 2019):

https://openreview.net/forum?id=BJlBNZDaP4

can be run blind:

```
./fast-downward.py domain.pddl problem.pddl --search "astar(blind())"
```

with hmax, taking into account or ignoring the cost bound (flip true --> false):

```
--search "astar(hmax(transform=osp_utility_to_cost(), use_cost_bound=true))"
```

or with merge and shrink (note that settings other than the ones here may not be correctly implemented):

```
--heuristic "ms=merge_and_shrink(shrink_strategy=shrink_bisimulation(greedy=false),merge_strategy=merge_sccs(order_of_sccs=topological,merge_selector=score_based_filtering(scoring_functions=[goal_relevance,dfp,total_order(atomic_before_product=false,atomic_ts_order=reverse_level,product_ts_order=new_to_old)])),label_reduction=exact(before_shrinking=true,before_merging=false),max_states=50000,threshold_before_merge=1,transform=osp_utility_to_cost(), use_cost_bound=true) --search "astar(ms)"
```

Note that the input pddl is expected to contain `:utility` and `:bound` settings, such as those used in the suite of problems available here: 

https://zenodo.org/record/2576024