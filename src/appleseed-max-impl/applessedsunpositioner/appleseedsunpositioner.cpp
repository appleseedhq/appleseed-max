#include "appleseedsunpositioner.h"

#include "appleseedenvmap/appleseedenvmap.h"

SunPositionerWrapper::SunPositionerWrapper()
: m_sun_positioner(nullptr)
, m_hour(12)
, m_minute(0)
, m_second(0)
, m_month(1)
, m_day(1)
, m_year(2020)
, m_timezone(0)
, m_north(0.0f)
, m_latitude(0.0f)
, m_longitude(0.0f)
{
}

SunPositionerWrapper::~SunPositionerWrapper()
{
}

float SunPositionerWrapper::get_phi() const
{
    return m_sun_positioner->get_azimuth();
}

float SunPositionerWrapper::get_theta() const
{
    return m_sun_positioner->get_zenith();
}

void SunPositionerWrapper::update(IParamBlock2* const pblock, TimeValue& t, Interval& params_validity)
{
    pblock->GetValueByName(L"hour", t, m_hour, params_validity);
    pblock->GetValueByName(L"minute", t, m_minute, params_validity);
    pblock->GetValueByName(L"second", t, m_second, params_validity);
    pblock->GetValueByName(L"month", t, m_month, params_validity);
    pblock->GetValueByName(L"day", t, m_day, params_validity);
    pblock->GetValueByName(L"year", t, m_year, params_validity);
    pblock->GetValueByName(L"timezone", t, m_timezone, params_validity);
    pblock->GetValueByName(L"north", t, m_north, params_validity);
    pblock->GetValueByName(L"latitude", t, m_latitude, params_validity);
    pblock->GetValueByName(L"longitude", t, m_longitude, params_validity);

    renderer::ParamArray                                    param;
    
    param.insert("hour", m_hour);
    param.insert("minute", m_minute);
    param.insert("second", m_second);
    param.insert("month", m_month);
    param.insert("day", m_day);
    param.insert("year", m_year);
    param.insert("timezone", m_timezone);
    param.insert("north", m_north);
    param.insert("latitude", m_latitude);
    param.insert("longitude", m_longitude);

    m_sun_positioner = renderer::SunPositionerFactory::create("Sun Positioner", param);

    m_sun_positioner->fetch_data();
    m_sun_positioner->compute_sun_position();
}
