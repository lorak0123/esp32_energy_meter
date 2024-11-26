#include <algorithm>

void medianFilter(float* input, float* output, int size, int windowSize) {
    int halfWindow = windowSize / 2;
    double* temp = new double[windowSize];

    for (int i = 0; i < size; i++) {
        int start = std::max(0, i - halfWindow);
        int end = std::min(size - 1, i + halfWindow);
        int count = end - start + 1;

        for (int j = 0; j < count; j++) {
            temp[j] = input[start + j];
        }

        std::sort(temp, temp + count);
        output[i] = temp[count / 2];
    }

    delete[] temp;
}

void movingAverage(float* input, float* output, int size, int windowSize) {
    int halfWindow = windowSize / 2;

    for (int i = 0; i < size; i++) {
        float sum = 0.0;
        int count = 0;

        for (int j = -halfWindow; j <= halfWindow; j++) {
            int index = i + j;
            if (index >= 0 && index < size) {
                sum += input[index];
                count++;
            }
        }
        output[i] = sum / count;
    }
}

void smoothData(float* voltage, int size, int medianWindow, int avgWindow) {
    float* temp = new float[size];

    medianFilter(voltage, temp, size, medianWindow);

    movingAverage(temp, voltage, size, avgWindow);

    delete[] temp;
}

float* VOLTAGE_ARRAY = new float[1000];
float* CURRENT_ARRAY = new float[1000];
float* TIMESTAMP_ARRAY = new float[1000];

unsigned short SAMPLE_START = 0;
unsigned short SAMPLE_END = 0;

float* calculatePowerData(Adafruit_ADS1115* voltage_sensor, Adafruit_ADS1115* current_sensor) {
    for (int i = 0; i < 1000; i++) {
        VOLTAGE_ARRAY[i] = voltage_sensor->getLastConversionResults() * VOLTAGE_SENSOR_SCALE;
        CURRENT_ARRAY[i] = current_sensor->getLastConversionResults() * CURRENT_SENSOR_SCALE;
        TIMESTAMP_ARRAY[i] = micros();
    }

    smoothData(VOLTAGE_ARRAY, 1000, 1, 2);
    smoothData(CURRENT_ARRAY, 1000, 1, 2);

    SAMPLE_START = 0;

    for (int i = 1; i < 1000; i++) {
        if (VOLTAGE_ARRAY[i] > VOLTAGE_ARRAY[i - 1] && VOLTAGE_ARRAY[i] > VOLTAGE_ARRAY[i + 1]) {
            SAMPLE_START = i;
            break;
        }
    }

    int cycle_end = 999;

    for (int i = 998; i > 0; i--) {
        if (VOLTAGE_ARRAY[i] > VOLTAGE_ARRAY[i - 1] && VOLTAGE_ARRAY[i] > VOLTAGE_ARRAY[i + 1]) {
            cycle_end = i;
            break;
        }
    }

    float powerP = 0;
    float powerS = 0;
    float powerQ = 0;
    float voltageRMS = 0;
    float currentRMS = 0;
    int cycles = 0;

    for (int i = SAMPLE_START; i < cycle_end; i++) {
        powerP += VOLTAGE_ARRAY[i] * CURRENT_ARRAY[i];
        voltageRMS += VOLTAGE_ARRAY[i] * VOLTAGE_ARRAY[i];
        currentRMS += CURRENT_ARRAY[i] * CURRENT_ARRAY[i];

        if (VOLTAGE_ARRAY[i] > VOLTAGE_ARRAY[i - 1] && VOLTAGE_ARRAY[i] > VOLTAGE_ARRAY[i + 1]) {
            cycles++;

            if (cycles == 4) {
                SAMPLE_END = i;
            }
        }
    }

    long test_duration = TIMESTAMP_ARRAY[cycle_end] - TIMESTAMP_ARRAY[SAMPLE_START];
    float frequency = cycles / (test_duration / 1000000.0);

    int count = cycle_end - SAMPLE_START;
    voltageRMS = sqrt(voltageRMS / count);
    currentRMS = sqrt(currentRMS / count);
    powerP /= count;
    powerS = voltageRMS * currentRMS;
    powerQ = sqrt(powerS * powerS - powerP * powerP);

    return new float[6]{powerP, powerS, powerQ, voltageRMS, currentRMS, frequency};
}