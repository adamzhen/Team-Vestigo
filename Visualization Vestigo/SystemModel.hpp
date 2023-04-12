#ifndef KALMAN_EXAMPLES1_ROBOT_SYSTEMMODEL_HPP_
#define KALMAN_EXAMPLES1_ROBOT_SYSTEMMODEL_HPP_

#include <kalman/LinearizedSystemModel.hpp>

namespace KalmanExamples
{
    namespace Vestigo
    {

        /**
         * @brief System state vector-type for a 3DOF planar robot
         *
         * This is a system state for a very simple planar robot that
         * is characterized by its (x,y)-Position and angular orientation.
         *
         * @param T Numeric scalar type
         */
        template<typename T>
        class State : public Kalman::Vector<T, 3>
        {
        public:
            KALMAN_VECTOR(State, T, 3)

                //! X-position
                static constexpr size_t X = 0;
            //! Y-Position
            static constexpr size_t Y = 1;
            //! Orientation
            static constexpr size_t THETA = 2;

            T x()       const { return (*this)[X]; }
            T y()       const { return (*this)[Y]; }
            T theta()   const { return (*this)[THETA]; }

            T& x() { return (*this)[X]; }
            T& y() { return (*this)[Y]; }
            T& theta() { return (*this)[THETA]; }
        };

        /**
         * @brief System control-input vector-type for a 3DOF planar robot
         *
         * This is the system control-input of a very simple planar robot that
         * can control the velocity in its current direction as well as the
         * change in direction.
         *
         * @param T Numeric scalar type
         */
        template<typename T>
        class Control : public Kalman::Vector<T, 3>
        {
        public:
            KALMAN_VECTOR(Control, T, 3)

                //! Velocity in the IMU's X (m/s)
                static constexpr size_t VX = 0;
            //! Velocity in the IMU's Y (m/s)
            static constexpr size_t VY = 1;
            //! Time delta t (s)
            static constexpr size_t DT = 2;

            T vx()  const { return (*this)[VX]; }
            T vy()  const { return (*this)[VY]; }
            T dt()  const { return (*this)[DT]; }

            T& vx() { return (*this)[VX]; }
            T& vy() { return (*this)[VY]; }
            T& dt() { return (*this)[DT]; }
        };

        /**
         * @brief System model for a simple planar 3DOF robot
         *
         * This is the system model defining how our robot moves from one
         * time-step to the next, i.e. how the system state evolves over time.
         *
         * @param T Numeric scalar type
         * @param CovarianceBase Class template to determine the covariance representation
         *                       (as covariance matrix (StandardBase) or as lower-triangular
         *                       coveriace square root (SquareRootBase))
         */
        template<typename T, template<class> class CovarianceBase = Kalman::StandardBase>
        class SystemModel : public Kalman::LinearizedSystemModel<State<T>, Control<T>, CovarianceBase>
        {
        public:
            //! State type shortcut definition
            typedef KalmanExamples::Vestigo::State<T> S;

            //! Control type shortcut definition
            typedef KalmanExamples::Vestigo::Control<T> C;

            /**
             * @brief Definition of (non-linear) state transition function
             *
             * This function defines how the system state is propagated through time,
             * i.e. it defines in which state \f$\hat{x}_{k+1}\f$ is system is expected to
             * be in time-step \f$k+1\f$ given the current state \f$x_k\f$ in step \f$k\f$ and
             * the system control input \f$u\f$.
             *
             * @param [in] x The system state in current time-step
             * @param [in] u The control vector input
             * @returns The (predicted) system state in the next time-step
             */
            S f(const S& x, const C& u) const
            {
                //! Predicted state vector after transition
                S x_;

                // Theta remains the same (for now)
                x_.theta() = x.theta();

                double PI = M_PI;
                // Switching velocity from IMU to reference coordinate system, then calculating x and y displacements
                x_.x() = x.x() + (std::cos(x.theta()) * u.vy() + std::cos(x.theta() - PI / 2) * u.vx()) * u.dt();
                x_.y() = x.y() + (std::sin(x.theta()) * u.vy() + std::sin(x.theta() - PI / 2) * u.vx()) * u.dt();

                // Return transitioned state vector
                return x_;
            }

        protected:
            /**
             * @brief Update jacobian matrices for the system state transition function using current state
             *
             * This will re-compute the (state-dependent) elements of the jacobian matrices
             * to linearize the non-linear state transition function \f$f(x,u)\f$ around the
             * current state \f$x\f$.
             *
             * @note This is only needed when implementing a LinearizedSystemModel,
             *       for usage with an ExtendedKalmanFilter or SquareRootExtendedKalmanFilter.
             *       When using a fully non-linear filter such as the UnscentedKalmanFilter
             *       or its square-root form then this is not needed.
             *
             * @param x The current system state around which to linearize
             * @param u The current system control input
             */
            void updateJacobians(const S& x, const C& u)
            {
                // F = df/dx (Jacobian of state transition w.r.t. the state)
                this->F.setZero();

                // partial derivative of x.x() w.r.t. x.x()
                this->F(S::X, S::X) = 1;
                // partial derivative of x.x() w.r.t. x.theta()
                this->F(S::X, S::THETA) = (-std::sin(x.theta()) * u.vy() + std::cos(x.theta()) * u.vx()) / u.dt();

                // partial derivative of x.y() w.r.t. x.y()
                this->F(S::Y, S::Y) = 1;
                // partial derivative of x.y() w.r.t. x.theta()
                this->F(S::Y, S::THETA) = (std::cos(x.theta()) * u.vy() + std::sin(x.theta()) * u.vx()) / u.dt();

                // partial derivative of x.theta() w.r.t. x.theta()
                this->F(S::THETA, S::THETA) = 1;

                // W = df/dw (Jacobian of state transition w.r.t. the noise)
                this->W.setIdentity();
                // TODO: more sophisticated noise modelling
                //       i.e. The noise affects the the direction in which we move as 
                //       well as the velocity (i.e. the distance we move)
            }
        };

    } // namespace Robot
} // namespace KalmanExamples

#endif