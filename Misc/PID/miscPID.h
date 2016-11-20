// Andrew Davies

#if !defined( MISC_PID_H )
#define MISC_PID_H

namespace Misc
{

struct PIDParameters
{
    float p_gain;      /* 'P' proportional gain          */
    float i_gain;      /* 'I' integral gain              */
    float d_gain;      /* 'D' derivative gain            */
    float vel_ff;      /* 'V' velocity feed forward      */
    float bias;        /* 'B' bias                       */
    float setpt;       /* 'S' set point                  */
    float min;         /* 'N' minimum output value       */
    float max;         /* 'M' maximum output value       */
    float slew;        /* 'W' maximum slew rate          */
};

class PID
{
private:
    float m_integral;
    float m_last_error;
    float m_last_output;

    PIDParameters m_parameters;

public:
    PID();

    PID( const PIDParameters& parameters );

    void set( const PIDParameters& parameters );

    void clear();

    float integral() const
    {
        return m_integral;
    }

    float last_error() const
    {
        return m_last_error;
    }

    float last_output() const
    {
        return m_last_output;
    }

    float compute( float current, float target );
};



}

#endif