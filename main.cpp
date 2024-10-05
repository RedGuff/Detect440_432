#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <complex>

using namespace std;

const double LA_440 = 440.0;
const double LA_432 = 432.0;
const double TOLERANCE = 1.0;

// Function to read a stereo WAV file and merge channels
vector<double> read_wav(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file " << filename << endl;
        exit(1);
    }

    // Skip the WAV header (44 bytes)
    file.seekg(44);

    vector<double> samples;
    char buffer[4];  // Buffer to hold left and right channel samples (2 bytes each)

    // Read stereo samples and merge them into mono
    while (file.read(buffer, sizeof(buffer))) {
        // Interpret buffer as two 16-bit signed integers (left and right channels)
        int16_t left_channel = *reinterpret_cast<int16_t*>(buffer);
        int16_t right_channel = *reinterpret_cast<int16_t*>(buffer + 2);

        // Merge the two channels into one (mono)
        double merged_sample = (left_channel + right_channel) / 2.0;
        samples.push_back(merged_sample / 32768.0); // Normalize to range [-1, 1]
    }

    file.close();
    return samples;
}

// Apply a Hamming window to the signal to reduce spectral leakage
void apply_hamming_window(vector<double>& data) {
    size_t N = data.size();
    for (size_t i = 0; i < N; ++i) {
        data[i] *= 0.54 - 0.46 * cos(2 * M_PI * i / (N - 1));
    }
}

// Basic FFT implementation (Cooley-Tukey)
void fft(vector<complex<double>>& data) {
    const size_t N = data.size();
    if (N <= 1) return;

    vector<complex<double>> even(N / 2), odd(N / 2);
    for (size_t i = 0; i < N / 2; ++i) {
        even[i] = data[i * 2];
        odd[i] = data[i * 2 + 1];
    }

    fft(even);
    fft(odd);

    for (size_t i = 0; i < N / 2; ++i) {
        complex<double> t = polar(1.0, -2 * M_PI * i / N) * odd[i];
        data[i] = even[i] + t;
        data[i + N / 2] = even[i] - t;
    }
}

// Function to find the dominant frequency in the spectrum
double find_dominant_frequency(const vector<double>& samples, int sample_rate) {
    // Use a segment of the samples (next power of 2)
    size_t N = 1 << static_cast<int>(log2(samples.size()));

    // Apply Hamming window before FFT
    vector<double> windowed_samples = samples;
    apply_hamming_window(windowed_samples);  // Apply window to the copied samples

    // Prepare complex data for FFT
    vector<complex<double>> data(N);

    // Copy the windowed samples into complex data
    for (size_t i = 0; i < N; ++i) {
        data[i] = complex<double>(windowed_samples[i], 0.0);
    }

    // Perform FFT
    fft(data);

    // Find the frequency with the maximum magnitude
    double max_magnitude = 0.0;
    size_t max_index = 0;
    for (size_t i = 0; i < N / 2; ++i) {
        double magnitude = abs(data[i]);
        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            max_index = i;
        }
    }

    // Convert index to frequency
    return (max_index * sample_rate) / double(N);
}

// Function to determine if the dominant frequency matches LA440 or LA432
void check_tuning(double frequency) {
    if (abs(frequency - LA_440) <= TOLERANCE) {
        cout << "The track is tuned to A440 Hz." << endl;
    } else if (abs(frequency - LA_432) <= TOLERANCE) {
        cout << "The track is tuned to A432 Hz." << endl;
    } else {
        cout << "The track is not tuned to A440 or A432 Hz." << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <audio_file.wav>" << endl;
        return 1;
    }

    string filename = argv[1];
    int sample_rate = 44100; // For simplicity, assume 44.1 kHz

    cout << "File name is: " << filename << endl;  // Debug line to display the file name

    // Step 1: Read the WAV file
    vector<double> samples = read_wav(filename);

    // Step 2: Find the dominant frequency
    double dominant_frequency = find_dominant_frequency(samples, sample_rate);
cout << "Dominant frequency around 440Hz = " << dominant_frequency << endl;
cout << "Don't worry about a slight deviation from your planned frequency: the calculations are approximate." << endl;
    // Step 3: Check if it matches A440 or A432
    check_tuning(dominant_frequency);

    return 0;
}
