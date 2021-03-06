// Andrew Davies

#include "pch.h"
#include "Misc/PID/miscPID.h"
#include <math.h>
#include "Misc/misc.h"

namespace Misc
{

const PIDParameters g_PIDParameters =
{
   0.05f,              /* 'P' proportional gain          */
   0.05f,              /* 'I' integral gain              */
   0.025f,              /* 'D' derivative gain            */
   0.00f,              /* 'V' velocity feed forward      */
   0.00f,              /* 'B' bias                       */
   60.0f,              /* 'S' set point                  */
   -100.0f,              /* 'N' minimum output value       */
   300.0f,              /* 'M' maximum output value       */
   0.0f,              /* 'W' slew limit                 */
};

PID::PID( )
    : m_integral( 0.0f )
    , m_last_error( 0.0f )
    , m_last_output( 0.0f )
    , m_parameters( g_PIDParameters )
{
}

PID::PID( const PIDParameters& parameters )
    : m_integral( 0.0f )
    , m_last_error( 0.0f )
    , m_last_output( 0.0f )
    , m_parameters( parameters )
{
}   

void PID::set( const PIDParameters& parameters )
{
    m_integral = 0.0f;
    m_last_error = 0.0f;
    m_last_output = 0.0f;
    m_parameters = parameters;
}   

void PID::clear( )
{
    m_integral = 0.0f;
    m_last_error = 0.0f;
    m_last_output = 0.0f;
}   

float PID::compute( const float current, const float target )
{
    /* the error for the current pass is the difference   */
    /* between the current target and the current PV      */
    const float this_error = target - current;

    /* the derivative is the difference between the error */
    /* for the current pass and the previous pass         */
    const float deriv = this_error - m_last_error;

    /* the new error is added to the integral             */
    m_integral += target - current;

    /* now that all of the variable terms have been       */
    /* computed they can be multiplied by the appropriate */
    /* coefficients and the resulting products summed     */
    float this_output = 
                m_parameters.p_gain * this_error
              + m_parameters.i_gain * m_integral
              + m_parameters.d_gain * deriv
              + m_parameters.vel_ff * target
              + m_parameters.bias;

    m_last_error = this_error;

    /* check for slew rate limiting on the output change  */
    if( fabsf( m_parameters.slew ) > FLT_ZERO )
    {
        if( this_output - m_last_output > m_parameters.slew)
        {
            this_output = m_last_output + m_parameters.slew;
        }
        else if( m_last_output - this_output > m_parameters.slew)
        {
            this_output = m_last_output - m_parameters.slew;
        }
    }

    /* now check the output value for absolute limits     */
    this_output = clamp( this_output, m_parameters.min, m_parameters.max );

    /* store the new output value to be used as the old   */
    /* output value on the next loop pass                 */
    m_last_output = this_output;
    return this_output;
}

}
