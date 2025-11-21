#pragma once

class KalmanFilter {
private:
    float x[2]; // state: [altitude, vertical_speed]
    float P[4]; // covariance matrix
    float Q[4]; // process noise covariance
    float R;    // measurement noise covariance
    float dt;   // time step

public:
    KalmanFilter(float dt_val, float process_noise, float measurement_noise);
    void predict();
    void update(float z);
    float getAltitude();
    float getVerticalSpeed();
    void setInitialState(float alt, float vs);
};