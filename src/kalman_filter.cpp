#include "kalman_filter.h"
#include <cmath> // For sqrt if needed, but not here

KalmanFilter::KalmanFilter(float dt_val, float process_noise, float measurement_noise) : dt(dt_val), R(measurement_noise) {
    // Initialize state
    x[0] = 0.0f; // altitude
    x[1] = 0.0f; // vertical_speed

    // Initialize covariance matrix P (2x2) as identity * large value for initial uncertainty
    P[0] = 100.0f; // P11
    P[1] = 0.0f;   // P12
    P[2] = 0.0f;   // P21
    P[3] = 100.0f; // P22

    // Initialize process noise covariance Q (2x2)
    // Assuming process noise affects both altitude and speed
    Q[0] = process_noise; // Q11
    Q[1] = 0.0f;          // Q12
    Q[2] = 0.0f;          // Q21
    Q[3] = process_noise; // Q22
}

void KalmanFilter::predict() {
    // State transition: x = F * x
    // F = [[1, dt], [0, 1]]
    float x_pred[2];
    x_pred[0] = x[0] + dt * x[1];
    x_pred[1] = x[1];

    // Covariance prediction: P = F * P * F^T + Q
    // F * P
    float FP[4];
    FP[0] = P[0] + dt * P[2]; // F11*P11 + F12*P21
    FP[1] = P[1] + dt * P[3]; // F11*P12 + F12*P22
    FP[2] = P[2];             // F21*P11 + F22*P21
    FP[3] = P[3];             // F21*P12 + F22*P22

    // (F*P) * F^T
    float FPFt[4];
    FPFt[0] = FP[0] + dt * FP[2]; // FP11 + dt*FP21
    FPFt[1] = FP[1] + dt * FP[3]; // FP12 + dt*FP22
    FPFt[2] = FP[2];              // FP21
    FPFt[3] = FP[3];              // FP22

    // P_pred = FPFt + Q
    P[0] = FPFt[0] + Q[0];
    P[1] = FPFt[1] + Q[1];
    P[2] = FPFt[2] + Q[2];
    P[3] = FPFt[3] + Q[3];

    // Update state
    x[0] = x_pred[0];
    x[1] = x_pred[1];
}

void KalmanFilter::update(float z) {
    // Measurement residual: y = z - H * x
    // H = [1, 0] for altitude measurement
    float y = z - x[0];

    // Innovation covariance: S = H * P * H^T + R
    // H * P = [P11, P12]
    float HP[2] = {P[0], P[1]};
    // H * P * H^T = HP[0]
    float S = HP[0] + R;

    // Kalman gain: K = P * H^T / S
    float K[2];
    K[0] = P[0] / S; // P11 / S
    K[1] = P[2] / S; // P21 / S

    // Update state: x = x + K * y
    x[0] += K[0] * y;
    x[1] += K[1] * y;

    // Update covariance: P = (I - K * H) * P
    // I - K*H = [[1 - K0, 0], [-K1, 1]]
    float IKH[4];
    IKH[0] = 1.0f - K[0]; // 1 - K0
    IKH[1] = 0.0f;
    IKH[2] = -K[1];       // -K1
    IKH[3] = 1.0f;

    // (I - K*H) * P
    float P_new[4];
    P_new[0] = IKH[0] * P[0] + IKH[1] * P[2];
    P_new[1] = IKH[0] * P[1] + IKH[1] * P[3];
    P_new[2] = IKH[2] * P[0] + IKH[3] * P[2];
    P_new[3] = IKH[2] * P[1] + IKH[3] * P[3];

    // Copy back
    P[0] = P_new[0];
    P[1] = P_new[1];
    P[2] = P_new[2];
    P[3] = P_new[3];
}

float KalmanFilter::getAltitude() {
    return x[0];
}

float KalmanFilter::getVerticalSpeed() {
    return x[1];
}

void KalmanFilter::setInitialState(float alt, float vs) {
    x[0] = alt;
    x[1] = vs;
    // Reset covariance to initial values
    P[0] = 100.0f;
    P[1] = 0.0f;
    P[2] = 0.0f;
    P[3] = 100.0f;
}