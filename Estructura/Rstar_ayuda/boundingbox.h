#pragma once
#include <algorithm>
#include <limits>
#include <vector>
enum class axis_type { upper, lower };

/*!
 * \brief Template class of the bounding box
 * \param[in] dimensions -  number of dimensions
 *  */
template <std::size_t dimensions> struct RStarBoundingBox {
    /*!
     * \brief An instance is created and filled with extreme values
     *  */
    RStarBoundingBox() : max_edges(dimensions), min_edges(dimensions) {
        for (size_t axis = 0; axis < dimensions; axis++) {
            max_edges[axis] = std::numeric_limits<int>::min();
            min_edges[axis] = std::numeric_limits<int>::max();
        }
    }
    ~RStarBoundingBox() = default;
    bool operator<(const RStarBoundingBox &rhs) const {
        if (min_edges == rhs.min_edges)
            return max_edges < rhs.max_edges;
        else
            return min_edges < rhs.min_edges;
    }
    bool operator==(const RStarBoundingBox &rhs) const {
        return max_edges == max_edges && min_edges == min_edges;
    }
    bool operator!=(const RStarBoundingBox &rhs) const {
        return !operator==(rhs);
    }

    /*!
     * \brief Sets the area edges to the default values
     *  */
    void reset() {
        for (size_t axis = 0; axis < dimensions; axis++) {
            max_edges[axis] = std::numeric_limits<int>::min();
            min_edges[axis] = std::numeric_limits<int>::max();
        }
    }

    /*!
     * \brief Selects the edges so that the other area fits completely within it
     * \param[in] other_box - other region
     *  */
    void stretch(const RStarBoundingBox<dimensions> &other_box) {
        for (size_t axis = 0; axis < dimensions;
                 axis++) { // Selects borders so that another area can be accommodated
            max_edges[axis] = std::max(max_edges[axis], other_box.max_edges[axis]);
            min_edges[axis] = std::min(min_edges[axis], other_box.min_edges[axis]);
        }
    }

    /*!
     * \brief  Returns true if the areas overlap
     * \param[in] other_box - second region
     *  */
    bool is_intersected(const RStarBoundingBox<dimensions> &other_box) const {
        if (overlap(other_box) > 0)
            return true; // Returns true if the intersection is greater than 0
        else
            return false;
    }

    /*!
     * \brief Returns the half-perimeter of the bounding box
     *  */
    int margin() const {
        int ans = 0;
        for (size_t axis = 0; axis < dimensions;
                 axis++) { // Calculates the sum of the umbrellas
            ans += max_edges[axis] - min_edges[axis];
        }
        return ans;
    }

    /*!
     * \brief Returns the area of the bounding box
     *  */
    int area() const { // Calculates surface area
        int ans = 1;
        for (size_t axis = 0; axis < dimensions; axis++) {
            ans *= max_edges[axis] - min_edges[axis];
        }
        return ans;
    }

    /*!
     * \brief Returns the intersection of two areas
     *  */
    int overlap(const RStarBoundingBox<dimensions> &other_box) const {
        int ans = 1;
        for (size_t axis = 0; axis < dimensions; axis++) {
            int x1 = min_edges[axis];           // lower limit
            int x2 = max_edges[axis];           // larger limit
            int y1 = other_box.min_edges[axis]; // smallest limit
            int y2 = other_box.max_edges[axis]; // larger limit

            if (x1 < y1) {
                if (y1 < x2) {
                    if (y2 < x2) {
                        ans *= (y2 - y1);
                    } else {
                        ans *= (x2 - y1);
                    }
                } else {
                    return 0;
                }
            } else {
                if (y2 > x1) {
                    if (y2 > x2) {
                        ans *= (x2 - x1);
                    } else {
                        ans *= (y2 - x1);
                    }
                } else {
                    return 0; // if it doesn't fit into any condition.
                }
            }
        }
        return ans;
    }

    /*!
     * \brief  Returns the distance between the centers of the two areas
     *  */
    double
    dist_between_centers(const RStarBoundingBox<dimensions> &other_box) const {
        // The result is the distance squared so as not to lose accuracy from the
        // square root.
        int ans = 0;
        for (size_t axis = 0; axis < dimensions; axis++) {
            int d = ((max_edges[axis] + min_edges[axis]) -
                             (other_box.max_edges[axis] + other_box.min_edges[axis])) /
                            2;
            ans += d * d;
        }
        return ans;
    }

    int value_of_axis(const int axis, const axis_type type) const {
        if (type == axis_type::lower) {
            return min_edges[axis];
        } else {
            return max_edges[axis];
        }
    }
    std::vector<int> max_edges, min_edges; //<borders
};