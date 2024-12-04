#include <SFML/Graphics.hpp>  
#include <iostream>  
#include <vector>  
#include <sstream>  
#include <thread>  
#include <chrono>  
#include <fstream>  
#include <stdexcept>  
#include <string>  
#include <windows.h>   // For Win32 API functions  

using namespace std;

class Grid {
private:
    vector<vector<bool>> cells;
    int rows;
    int cols;

    int countLivingNeighbors(int x, int y) {
        int livingNeighbors = 0;
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                if (i == 0 && j == 0) continue;

                int newX = x + i;
                int newY = y + j;
                if (newX >= 0 && newX < rows && newY >= 0 && newY < cols) {
                    if (cells[newX][newY]) {
                        livingNeighbors++;
                    }
                }
            }
        }
        return livingNeighbors;
    }

public:
    Grid(int r, int c) : rows(r), cols(c) {
        cells.resize(rows, vector<bool>(cols, false));
    }

    void initializeFromInput(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("Could not open file.");
        }
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                int state;
                file >> state;
                cells[i][j] = (state == 1);
            }
        }
        file.close();
    }

    void update() {
        vector<vector<bool>> newCells = cells;

        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                int livingNeighbors = countLivingNeighbors(i, j);
                if (cells[i][j]) {
                    if (livingNeighbors < 2 || livingNeighbors > 3) {
                        newCells[i][j] = false;
                    }
                }
                else {
                    if (livingNeighbors == 3) {
                        newCells[i][j] = true;
                    }
                }
            }
        }
        cells = newCells;
    }

    void draw(sf::RenderWindow& window, int cellSize) const {
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                sf::RectangleShape cell(sf::Vector2f(cellSize, cellSize));
                cell.setPosition(j * cellSize, i * cellSize);
                cell.setFillColor(cells[i][j] ? sf::Color::White : sf::Color::Black);
                window.draw(cell);
            }
        }
    }

    void toggleCell(int mouseX, int mouseY, int cellSize) {
        int col = mouseX / cellSize;
        int row = mouseY / cellSize;

        if (row >= 0 && row < rows && col >= 0 && col < cols) {
            cells[row][col] = !cells[row][col];
        }
    }

    void print() const {
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                cout << (cells[i][j] ? "1 " : "0 ");
            }
            cout << endl;
        }
        cout << "-----------------------" << endl;
    }

    void saveToFile(const string& directory, const string& baseFilename, int generation) const {
        stringstream ss;
        ss << directory << "\\" << baseFilename << "_gen_" << generation << ".txt";

        ofstream outFile(ss.str());
        if (!outFile) {
            cerr << "Erreur : Impossible d'ouvrir le fichier de sortie." << endl;
            return;
        }
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                outFile << (cells[i][j] ? "1 " : "0 ");
            }
            outFile << endl;
        }
        outFile.close();
    }
};

class StartButton {
private:
    sf::RectangleShape button;
    sf::Text buttonText;
    bool isStarted;
    bool isDisabled;

public:
    StartButton(float x, float y, float width, float height, sf::Font& font)
        : isStarted(false), isDisabled(false) {
        button.setPosition(x, y);
        button.setSize(sf::Vector2f(width, height));
        button.setFillColor(sf::Color::Green);

        buttonText.setFont(font);
        buttonText.setString("Démarrer");
        buttonText.setCharacterSize(24);
        buttonText.setFillColor(sf::Color::Black);
        buttonText.setPosition(x + 10, y + 5);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(button);
        window.draw(buttonText);
    }

    bool isClicked(int mouseX, int mouseY) {
        return button.getGlobalBounds().contains(mouseX, mouseY);
    }

    void startSimulation() {
        isStarted = true;
        buttonText.setString("Pause");
        button.setFillColor(sf::Color::Red);
    }

    void pauseSimulation() {
        isStarted = false;
        buttonText.setString("Reprendre");
        button.setFillColor(sf::Color::Blue);
    }

    void disableButton() {
        isDisabled = true;
        buttonText.setString("Nombre d'itérations max atteint");
        button.setFillColor(sf::Color::Magenta);
    }

    bool getIsStarted() const {
        return isStarted;
    }

    bool getIsDisabled() const {
        return isDisabled;
    }
};

// Function to check if a directory exists using Win32 API  
bool directoryExists(const string& path) {
    DWORD ftyp = GetFileAttributesA(path.c_str());
    return (ftyp != INVALID_FILE_ATTRIBUTES && (ftyp & FILE_ATTRIBUTE_DIRECTORY));
}

// Function to create a directory using Win32 API  
bool createDirectory(const string& path) {
    return CreateDirectoryA(path.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

int main() {
    int rows = 5; // Nombre de lignes  
    int cols = 10; // Nombre de colonnes  
    int cellSize = 50; // Taille de chaque cellule  
    int maxIterations = 0; // Nombre d'itérations maximal  
    int iterationsCount = 0; // Compteur d'itérations  
    string mode;
    string filename;

    cout << "Choisissez le mode (terminal ou graphique) : ";
    cin >> mode;

    if (mode != "terminal" && mode != "graphique") {
        cout << "Mode invalide, veuillez redémarrer et choisir 'terminal' ou 'graphique'." << endl;
        return 1;
    }

    Grid grid(rows, cols);
    sf::RenderWindow window;
    sf::Font font;

    if (mode == "graphique") {
        cout << "Entrez le nombre d'itérations (0 pour une simulation infinie) : ";
        cin >> maxIterations;
        window.create(sf::VideoMode(cols * cellSize, rows * cellSize + 50), "Jeu de la Vie");

        if (!font.loadFromFile("arial.ttf")) {
            cerr << "Erreur lors du chargement de la police" << endl;
            return -1;
        }

        StartButton startButton(10, rows * cellSize + 10, 150, 30, font);
        bool simulationRunning = false;

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }

                if (event.type == sf::Event::MouseButtonPressed) {
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        if (startButton.isClicked(event.mouseButton.x, event.mouseButton.y)) {
                            if (!startButton.getIsDisabled()) {
                                if (simulationRunning) {
                                    startButton.pauseSimulation();
                                    simulationRunning = false;
                                }
                                else {
                                    startButton.startSimulation();
                                    simulationRunning = true;
                                    iterationsCount = 0;
                                }
                            }
                        }
                        else {
                            if (!simulationRunning) {
                                grid.toggleCell(event.mouseButton.x, event.mouseButton.y, cellSize);
                            }
                        }
                    }
                }
            }

            if (simulationRunning) {
                grid.update();
                iterationsCount++;
                if (maxIterations > 0 && iterationsCount >= maxIterations) {
                    startButton.disableButton();
                    simulationRunning = false;
                }
            }

            window.clear(sf::Color::Black);
            grid.draw(window, cellSize);
            startButton.draw(window);
            window.display();

            if (simulationRunning) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
    else {
        cout << "Entrez le nom du fichier d'entrée : ";
        cin >> filename;

        ifstream infile(filename);
        if (!infile) {
            cerr << "Erreur : Impossible d'ouvrir le fichier." << endl;
            return 1;
        }

        infile >> rows >> cols;
        grid = Grid(rows, cols);
        grid.initializeFromInput(filename);
        infile.close();

        cout << "État initial de la grille : " << endl;
        grid.print();

        int generations;
        cout << "Entrez le nombre de générations à simuler : ";
        cin >> generations;

        // Create output directory based on input filename  
        string outputDir = filename + "_out";

        // Check if the directory exists, if not, create it using Win32 API  
        if (!directoryExists(outputDir)) {
            if (createDirectory(outputDir)) {
                cout << "Le répertoire de sortie a été créé : " << outputDir << endl;
            }
            else {
                cerr << "Erreur : Impossible de créer le répertoire de sortie." << endl;
                return 1; // Exit if directory creation fails  
            }
        }
        else {
            cout << "Le répertoire de sortie existe déjà : " << outputDir << endl;
        }

        for (int generation = 0; generation < generations; ++generation) {
            grid.update();
            cout << "État de la grille à la génération " << generation + 1 << " : " << endl;
            grid.print();

            // Save each generation to file  
            grid.saveToFile(outputDir, filename, generation + 1);
        }
    }

    return 0;
}