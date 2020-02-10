#pragma once

namespace spt::geo {
    template<typename T>
    struct Vector2D {
        T x = 0, y = 0;
    };

    template<typename T>
    class Box2D {
    private:
        typedef Box2D<T> Self;
    public:
        typedef T Metric;

        T xmin = 0, ymin = 0, xmax = 0, ymax = 0;

        inline T Area() const {
            return Width() * Height();
        }

        inline T Width() const {
            return xmax - xmin;
        }

        inline T Height() const {
            return ymax - ymin;
        }

        bool operator&&(const Self &other) const {
            return xmax > other.xmin &&
                   xmin < other.xmax &&
                   ymax > other.ymin &&
                   ymin < other.ymax;
        }

        bool operator&&(const Vector2D<T> &p) const {
            return xmax >= p.x &&
                   xmin <= p.x &&
                   ymax >= p.y &&
                   ymin <= p.y;
        }

        Self operator+(const Self &other) const {
            return {
                    std::min(xmin, other.xmin), std::min(ymin, other.ymin),
                    std::max(xmax, other.xmax), std::max(ymax, other.ymax)
            };
        }

        friend std::ostream &operator<<(std::ostream &os, const Self &box) {
            os << box.xmin << " " << box.ymin << " " << box.xmax << " " << box.ymax;
            return os;
        }
    };
}
