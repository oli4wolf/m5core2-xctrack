#include "sound.h"
#include "config.h"
#include <math.h> // For fabs()

// Initialize the speaker
void initSound() {
    M5.Speaker.begin();
    M5.Speaker.setVolume(SPEAKER_DEFAULT_VOLUME);
}

// Play tone based on vertical speed
void playTone(float verticalSpeed) {
    if (verticalSpeed > ALTITUDE_CHANGE_THRESHOLD_MPS) {
        // Rising tone: higher frequency, frequency increases with climb rate
        int frequency = RISING_TONE_BASE_FREQ_HZ + (int)(verticalSpeed * RISING_TONE_MULTIPLIER_HZ_PER_MPS);
        M5.Speaker.tone(frequency, TONE_DURATION_MS);
    } else if (verticalSpeed < -ALTITUDE_CHANGE_THRESHOLD_MPS) {
        // Sinking tone: lower frequency, frequency decreases with sink rate
        int frequency = SINKING_TONE_BASE_FREQ_HZ - (int)(fabs(verticalSpeed) * SINKING_TONE_MULTIPLIER_HZ_PER_MPS);
        if (frequency < MIN_TONE_FREQ_HZ) frequency = MIN_TONE_FREQ_HZ;
        M5.Speaker.tone(frequency, TONE_DURATION_MS);
    } else {
        // Stable or minor changes, no tone
        M5.Speaker.stop();
    }
}

// Stop the sound
void stopSound() {
    M5.Speaker.stop();
}