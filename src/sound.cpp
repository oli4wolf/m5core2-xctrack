#include "sound.h"
#include "config.h"
#include <math.h> // For fabs()

// Global sound enable flag (declared in main.cpp)
extern bool globalSoundEnabled;

// Initialize the speaker
void initSound() {
    M5.Speaker.begin();
    M5.Speaker.setVolume(SPEAKER_DEFAULT_VOLUME);
    ESP_LOGI("Sound", "Sound module initialized");
}

// Play tone based on vertical speed
void playTone(float verticalSpeed) {
    // Only play tone if sound is enabled
    if (!globalSoundEnabled) {
        M5.Speaker.stop();
        return;
    }

    if (verticalSpeed > ALTITUDE_CHANGE_THRESHOLD_MPS) {
        // Rising tone: higher frequency, frequency increases with climb rate
        int frequency = RISING_TONE_BASE_FREQ_HZ + (int)(verticalSpeed * RISING_TONE_MULTIPLIER_HZ_PER_MPS);
        if (frequency < MIN_TONE_FREQ_HZ) frequency = MIN_TONE_FREQ_HZ;
        else if (frequency > MAX_TONE_FREQ_HZ) frequency = MAX_TONE_FREQ_HZ;
        M5.Speaker.tone(frequency, TONE_DURATION_MS);
    } else if (verticalSpeed < -ALTITUDE_CHANGE_THRESHOLD_MPS) {
        // Sinking tone: lower frequency, frequency decreases with sink rate
        int frequency = SINKING_TONE_BASE_FREQ_HZ - (int)(fabs(verticalSpeed) * SINKING_TONE_MULTIPLIER_HZ_PER_MPS);
        if (frequency < MIN_TONE_FREQ_HZ) frequency = MIN_TONE_FREQ_HZ;
        else if (frequency > MAX_TONE_FREQ_HZ) frequency = MAX_TONE_FREQ_HZ;
        M5.Speaker.tone(frequency, TONE_DURATION_MS);
    } else {
        // Stable or minor changes, no tone
        M5.Speaker.stop();
    }
}

// Stop the sound
void stopTone() {
    M5.Speaker.stop();
}

// Set volume
void setVolume(int volume) {
    M5.Speaker.setVolume(volume);
}