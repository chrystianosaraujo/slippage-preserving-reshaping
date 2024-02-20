#pragma once

#include <string>

namespace reshaping {
    class TerminationCriterion {
    public:
        enum class TYPE {
            MAX_ITER          = 1 << 0, // 1
            MAX_VERTEX_CHANGE = 1 << 1, // 2
            NEGATIVE_DELTA    = 1 << 2, // 4
            ENERGY_DELTA_TOL  = 1 << 3  // 8
        };

        TerminationCriterion() {

        }

        TerminationCriterion(bool max_iter_reached,
                             bool max_vertex_changed_reached,
                             bool negative_delta_reached,
                             bool energy_delta_tol_reached) {

            set_criteria(max_iter_reached,
                         max_vertex_changed_reached,
                         negative_delta_reached,
                         energy_delta_tol_reached);
        }

        void add_criterion(TYPE t) {
            m_criteria_reached |= (int) t; 
        }

        void set_criteria(bool max_iter_reached,
                          bool max_vertex_changed_reached,
                          bool negative_delta_reached,
                          bool energy_delta_tol_reached) {
            if (max_iter_reached)
                add_criterion(TYPE::MAX_ITER);

            if (max_vertex_changed_reached)
                add_criterion(TYPE::MAX_VERTEX_CHANGE);

            if (negative_delta_reached)
                add_criterion(TYPE::NEGATIVE_DELTA);

            if (energy_delta_tol_reached)
                add_criterion(TYPE::ENERGY_DELTA_TOL);
        }

        bool criterion_reached(TYPE t) const {
            return (m_criteria_reached & (int) t) == (int) t; 
        }

        bool has_converged() const {
            return m_criteria_reached > 0;
        }

        std::string to_string() const {
            std::string out_str;

            auto concat_termination = [&out_str](const std::string& str) {
                if(!out_str.empty())
                    out_str += " | ";
                out_str += str;
            };

            if(criterion_reached(TYPE::MAX_ITER))
                concat_termination("MAX_ITER");

            if(criterion_reached(TYPE::MAX_VERTEX_CHANGE))
                concat_termination("MAX_VERTEX_CHANGE");

            if(criterion_reached(TYPE::NEGATIVE_DELTA))
                concat_termination("NEG_DELTA");

            if(criterion_reached(TYPE::ENERGY_DELTA_TOL))
                concat_termination("ENERGY_DELTA");

            if(out_str.empty())
                return "NONE";
            else
                return out_str;
        }

    private:
        int m_criteria_reached = 0;
    };
}
