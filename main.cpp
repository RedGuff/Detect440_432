#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <complex>
#include <sys/stat.h>
#include <unistd.h>
using namespace std;

const double LA_440 = 440.0;
const double LA_432 = 432.0;
const double TOLERANCE = 1.0;
const int SPEED_of_your_computer_Min = 5; // Speed in Mb/s, usually 5Mb/s.
const int SPEED_of_your_computer_Max = 10; // Speed in Mb/s, usually 10Mb/s.

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

    // Find the frequency with the maximum magnitude // Of other... ???
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
        cout << "The track is tuned to A4 = 440Hz." << endl;
    } else if (abs(frequency - LA_432) <= TOLERANCE) {
        cout << "The track is tuned to A4 = 432Hz." << endl;
    } else {
        cout << "The track seems not be tuned to A4 = 440Hz or 432Hz, or I'm stupid." << endl;
        cout << "I'm probably stupid." << endl;
    }
}

long long getFileSize(const string& filename) {
    ifstream file(filename, ios::binary); // Ouvre le fichier en mode binaire
    if (!file) {
       // cerr << "Erreur d'ouverture du fichier." << endl;
        cerr << "Impossible to open the file" << filename <<"!" << endl;
        return -1; // Erreur si le fichier n'existe pas
    }

    // Déplace le pointeur à la fin du fichier
    file.seekg(0, ios::end);
    // Récupère la position actuelle du pointeur (taille du fichier)
    long long size = file.tellg();
    file.close(); // Ferme le fichier
    return size; // Retourne la taille du fichier
}

string ajouterExtension(const string& nomFichier, string extension = ".wav") {
    if (nomFichier.size() >= extension.size() &&
            nomFichier.compare(nomFichier.size() - extension.size(), extension.size(), extension) == 0) {
        return nomFichier;  // L'extension est déjà présente
    } else {
        return nomFichier + extension;  // Ajoute l'extension.
    }
}

bool exists_test3 (const std::string& name) { // https://stackoverflow-com.translate.goog/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

double correction(double frequency) { // Horrible method, but why not.
    if (frequency<16) { // Included "negative". What a horrible mistake!
        cerr << "Infrasound!" << endl;
    } else if (frequency>20000) {
        cerr << "Ultrasound!" << endl;
    } else {
        clog << "Normal sound." << endl;
        while (frequency<400) {
            frequency = frequency*2;
        }
        while (frequency>600) {
            frequency = frequency/2;
        }
    }
    return frequency;
}

int main(int argc, char* argv[]) { // Ok.
    string filename0 = "";
    if (argc < 2) {
        //    cerr << "Usage: " << argv[0] << " <audio_file.wav>" << endl;
        cout << "Name of the file?" << endl;

        cin >>  filename0;
        // return 1;
    } else {
        filename0 = argv[1];
    }
    string filename = "";
    if (exists_test3(filename0)) {
        filename = filename0;
    } else if(exists_test3(ajouterExtension(filename0, ".wav"))) {
        filename=ajouterExtension(filename0, ".wav");
    } else if(exists_test3(ajouterExtension(filename0, ".mp3"))) {
        cout << "I found " << ajouterExtension(filename0, ".mp3");
        cerr << ", but I can't read MP3 files!" << endl;
        return -1;
    } else if(exists_test3(ajouterExtension(filename0, ".aac"))) {
        cout << "I found " << ajouterExtension(filename0, ".aac");
        cerr << ", but I can't read AAC files!" << endl;
        return -1;
    } else if(exists_test3(ajouterExtension(filename0, ".m4a"))) {
        cout << "I found " << ajouterExtension(filename0, ".m4a");
        cerr << ", but I can't read M4A files!" << endl;
        return -1;
    } else if(exists_test3(ajouterExtension(filename0, ".opus"))) {
        cout << "I found " << ajouterExtension(filename0, ".opus");
        cerr << ", but I can't read OPUS files!" << endl;
        return -1;
    } else {
        cerr << "File do NOT exist! I tried to add \".wav\", but I failed to find the file!" << endl;
        return -1;
    }

// filename0 can be deleted.
    cout << "File name is: " << filename << endl;  // Debug line to display the file name
    long long sizeL = getFileSize(filename);
    cout << "Size:" << sizeL << " octets." << endl;
    cout << "About " << SPEED_of_your_computer_Min << " to " << SPEED_of_your_computer_Max << "Mb/s to calculate. \It depends of the power of your computer." << endl;
    cout << "About " << sizeL/(SPEED_of_your_computer_Max *1000000.0) << " to " << sizeL/(SPEED_of_your_computer_Min *1000000.0)<< " seconds to calculate." << endl;
    cout << "Don't worry about a slight deviation from your planned frequency:\nthe calculations are approximate." << endl;


    int sample_rate = 44100; // For simplicity, assume 44.1 kHz
    // Step 1: Read the WAV file
    vector<double> samples = read_wav(filename);

    // Step 2: Find the dominant frequency
    double dominant_frequency = find_dominant_frequency(samples, sample_rate);
// cout << "Dominant frequency around 440Hz = " << dominant_frequency << endl;

    cout << "Dominant frequency = " << dominant_frequency << endl;
    double corrected_frequency = correction(dominant_frequency);
    // Step 3: Check if it matches A440 or A432
    cout << "Corrected frequency = " << corrected_frequency << endl;
    check_tuning(corrected_frequency);
    return 0;
}
