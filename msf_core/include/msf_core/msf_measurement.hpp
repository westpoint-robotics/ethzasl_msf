/*

Copyright (c) 2012, Simon Lynen, ASL, ETH Zurich, Switzerland
You can contact the author at <slynen at ethz dot ch>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of ETHZ-ASL nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ETHZ-ASL BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef MEASUREMENT_HPP_
#define MEASUREMENT_HPP_

#include <msf_core/msf_fwds.hpp>
#include <Eigen/Dense>
#include <msf_core/msf_statedef.hpp>

namespace msf_core{

/**
 * \brief The base class for all measurement types.
 * These are the objects provided to the EKF core to be applied in correct order to the states
 */
class MSF_MeasurementBase{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	typedef msf_core::EKFState state_T; ///<the EKF state type this measurement can be applied to
	virtual ~MSF_MeasurementBase(){}
	/**
	 * \brief the method called by the msf_core to apply the measurement represented by this object
	 */
	virtual void apply(boost::shared_ptr<EKFState> stateWithCovariance, MSF_Core& core) = 0;
	double time_; ///<the time this measurement was taken
protected:
	/**
	 * main update routine called by a given sensor, will apply the measurement to the state inside the core
	 */
	template<class H_type, class Res_type, class R_type>
	void calculateAndApplyCorrection(boost::shared_ptr<EKFState> state, MSF_Core& core, const Eigen::MatrixBase<H_type>& H_delayed,
			const Eigen::MatrixBase<Res_type> & res_delayed, const Eigen::MatrixBase<R_type>& R_delayed);
};

/**
 * \brief An invalid measurement needed for the measurement container to report if something went wrong
 */
class MSF_InvalidMeasurement:public MSF_MeasurementBase{
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	virtual void apply(boost::shared_ptr<EKFState> stateWithCovariance, MSF_Core& core){
		ROS_ERROR_STREAM("Called apply() on an MSF_InvalidMeasurement object. This should never happen.");
	}
};

/**
 * \brief The class for sensor based measurements which we want to apply to
 * a state in the update routine of the EKF. This calls the apply correction
 * method of the EKF core
 * \note provides an abstract NVI to create measurements from sensor readings
 */
template<typename T, int MEASUREMENTSIZE>
class MSF_Measurement: public MSF_MeasurementBase{
private:
	virtual void makeFromSensorReadingImpl(const boost::shared_ptr<T const> reading) = 0;
protected:
	Eigen::Matrix<double, MEASUREMENTSIZE, MEASUREMENTSIZE> R_;
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	typedef T Measurement_type;
	typedef boost::shared_ptr<T const> Measurement_ptr;

	virtual ~MSF_Measurement(){};
	void makeFromSensorReading(const boost::shared_ptr<T const> reading, double timestamp){
		time_ = timestamp;
		makeFromSensorReadingImpl(reading);
	}
	//apply is implemented by respective sensor measurement types
};

/**
 * \brief A measurement to be send to initialize parts of or the full EKF state
 * this can especially be used to split the initialization of the EKF
 * between multiple sensors which init different parts of the state
 */
class MSF_InitMeasurement:public MSF_MeasurementBase{
private:
	MSF_MeasurementBase::state_T InitState; ///< values for initialization of the state
	bool ContainsInitialSensorReadings_; ///<flag whether this measurement contains initial sensor readings
	typedef MSF_MeasurementBase::state_T::stateVector_T stateVector_T;
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
	MSF_InitMeasurement(bool ContainsInitialSensorReadings){
		ContainsInitialSensorReadings_ = ContainsInitialSensorReadings;
		time_ = ros::Time::now().toSec();
	}
	virtual ~MSF_InitMeasurement(){};

	MSF_MeasurementBase::state_T::P_type& get_P(){
		return InitState.P_;
	}
	/**
	 * \brief get the gyro measurment
	 */
	Eigen::Matrix<double, 3, 1>& get_w_m(){
		return InitState.w_m_;
	}
	/**
	 * \brief get the acceleration measurment
	 */
	Eigen::Matrix<double, 3, 1>& get_a_m(){
		return InitState.a_m_;
	}

	/**
	 * \brief set the flag that the state variable at index INDEX has init values for the state
	 */
	template<int INDEX, typename T>
	void setStateInitValue(const T& initvalue){
		InitState.getStateVar<INDEX>().state_ = initvalue;
		InitState.getStateVar<INDEX>().hasResetValue = true;
	}

	/**
	 * \brief reset the flag that the state variable at index INDEX has init values for the state
	 */
	template<int INDEX>
	void resetStateInitValue(){
		InitState.get<INDEX>().hasResetValue = false;
	}

	/**
	 * \brief get the value stored in this object to initialize a state variable at index INDEX
	 */
	template<int INDEX>
	const typename msf_tmp::StripReference<typename boost::fusion::result_of::at_c<stateVector_T, INDEX >::type>::result_t::value_t&
	getStateInitValue() const {
			return InitState.get<INDEX>();
		}

	virtual void apply(boost::shared_ptr<EKFState> stateWithCovariance, MSF_Core& core);
};


/**
 * \brief A comparator to sort measurements by time
 */
template<typename stateSequence_T>
class sortMeasurements
{
public:
	/**
	 * implements the sorting by time
	 */
	bool operator() (const MSF_MeasurementBase& lhs, const MSF_MeasurementBase&rhs) const
	{
		return (lhs.time_<rhs.time_);
	}
};

}

#include <msf_core/implementation/msf_measurement.hpp>

#endif /* MEASUREMENT_HPP_ */