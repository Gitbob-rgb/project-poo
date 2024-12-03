#include <SFML/Graphics.hpp>  
#include <iostream>  
#include <vector>  
#include <sstream> // Pour std::stringstream  
#include <thread> // Pour std::this_thread::sleep_for  
#include <chrono> // Pour std::chrono::milliseconds  

#include <fstream>  
#include <stdexcept>  


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

    void print() const { // Nouvelle m�thode pour imprimer la grille dans le terminal  
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                cout << (cells[i][j] ? "1 " : "0 ");
            }
            cout << endl;
        }
        cout << "-----------------------" << endl; // Ligne de s�paration pour les g�n�rations  
    }
};

class StartButton {
private:
    sf::RectangleShape button;
    sf::Text buttonText;
    bool isStarted;
    bool isDisabled; // �tat pour d�sactiver le bouton

public:
    StartButton(float x, float y, float width, float height, sf::Font& font) : isStarted(false), isDisabled(false) {
        button.setPosition(x, y);
        button.setSize(sf::Vector2f(width, height));
        button.setFillColor(sf::Color::Green);

        buttonText.setFont(font);
        buttonText.setString("D�marrer");
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
        buttonText.setString("Nombre d'it�rations max atteint");
        button.setFillColor(sf::Color::Magenta); // Change the button color to indicate that it is disabled  
    }

    bool getIsStarted() const {
        return isStarted;
    }

    bool getIsDisabled() const {
        return isDisabled; // This can help if needed in other logic  
    }
};

int main() {
    int rows = 5; // Nombre de lignes  
    int cols = 10; // Nombre de colonnes  
    int cellSize = 50; // Taille de chaque cellule  
    int maxIterations = 0; // Nombre d'it�rations maximal  
    int iterationsCount = 0; // Compteur d'it�rations  
    string mode;
    string filename;
    cout << "Choisissez le mode (terminal ou graphique) : ";
    cin >> mode;

    if (mode != "terminal" && mode != "graphique") {
        cout << "Mode invalide, veuillez red�marrer et choisir 'terminal' ou 'graphique'." << endl;
        return 1;
    }

    // Demande � l'utilisateur le nombre d'it�rations  
    

    Grid grid(rows, cols);

    // Configuration de la fen�tre graphique uniquement si le mode est graphique  
    sf::RenderWindow window;
    sf::Font font;

    if (mode == "graphique") {
        cout << "Entrez le nombre d'it�rations (0 pour une simulation infinie) : ";
        cin >> maxIterations;
        window.create(sf::VideoMode(cols * cellSize, rows * cellSize + 50), "Jeu de la Vie"); // Espace pour le bouton  

        if (!font.loadFromFile("arial.ttf")) { // Assurez-vous que "arial.ttf" est dans le m�me r�pertoire que le code  
            cerr << "Erreur lors du chargement de la police" << endl;
            return -1;
        }

        StartButton startButton(10, rows * cellSize + 10, 150, 30, font); // Cr�er le bouton  

        bool simulationRunning = false; // Indique si la simulation est en cours  

        // Boucle principale pour le mode graphique  
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }

                // D�tection du clic de souris  
                if (event.type == sf::Event::MouseButtonPressed) {
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        // V�rifier si le bouton est cliqu�  
                        if (startButton.isClicked(event.mouseButton.x, event.mouseButton.y)) {
                            if (!startButton.getIsDisabled()) {
                            if (simulationRunning) {
                                startButton.pauseSimulation(); // Met en pause la simulation  
                                simulationRunning = false; // Met � jour l'�tat  
                            }
                            else {
                                startButton.startSimulation(); // D�marre la simulation  
                                simulationRunning = true; // Met � jour l'�tat  
                                iterationsCount = 0; // R�initialiser le compteur d'it�rations  
                            }
                        }
                        }
                        else {
                            // Activer/d�sactiver une cellule si la simulation n'est pas encore commenc�e  
                            if (!simulationRunning) {
                                grid.toggleCell(event.mouseButton.x, event.mouseButton.y, cellSize);
                            }
                        }
                    }
                }
            }

            // Met � jour la grille uniquement si la simulation est en cours  
            if (simulationRunning) {
                grid.update(); // Applique les r�gles du jeu  
                iterationsCount++;
                if (maxIterations > 0 && iterationsCount >= maxIterations) {
                    startButton.disableButton();
                    simulationRunning = false; // Arr�te la simulation apr�s le nombre d'it�rations  
                }
            }

            // Effacement de la fen�tre  
            window.clear(sf::Color::Black);
            grid.draw(window, cellSize); // Dessiner la grille  
            startButton.draw(window); // Dessiner le bouton  
            window.display(); // Afficher le contenu de la fen�tre  

            // Attendre avant de passer � la prochaine g�n�ration (100 ms)  
            if (simulationRunning) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
    else { // Mode terminal  
        while (true) {
            cout << "Entrez le nom du fichier d'entr�e : ";
            cin >> filename;

            ifstream infile(filename);
            if (!infile) {
                cerr << "Erreur : Impossible d'ouvrir le fichier." << endl;
                return 1;
            }

            infile >> rows >> cols;
            Grid grid(rows, cols);
            grid.initializeFromInput(filename);
            infile.close();

            cout << "�tat initial de la grille : " << endl;
            grid.print();

            int generations;
            cout << "Entrez le nombre de g�n�rations � simuler : ";
            cin >> generations;

            for (int generation = 0; generation < generations; ++generation) {
                grid.update();
                cout << "�tat de la grille � la g�n�ration " << generation + 1 << " : " << endl;
                grid.print();
            }
            
            return 0;
            
              
        }
    }

    return 0;
}