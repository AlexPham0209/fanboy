#include <iostream>
#include <GameBoy.h>
#include <Cartridge/CartridgeFactory.h>
#include <imgui.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <backends/imgui_impl_sdl2.h>
#include <chrono>

#define SDL_MAIN_HANDLED 
#include <SDL.h>
#include <nfd.h>

const int SCALE = 4;
bool running = true;
bool isPaletteWindowOpen = false;
bool isControlWindowOpen = false;

std::map<int, unsigned char> keyMap;

Color color;

GameBoy* gameboy;
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;

bool initWindow() {
	SDL_Init(SDL_INIT_EVERYTHING);
	window = SDL_CreateWindow("Gameboy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * SCALE, 144 * SCALE, SDL_WINDOW_SHOWN);
	if (window == nullptr) {
		std::cout << "Failed to open window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" << std::endl;
		return false;
	}
	std::cout << "Window Created" << std::endl;

	renderer = SDL_CreateRenderer(window, -1, 0);

	if (renderer == nullptr) {
		std::cout << "Failed to create renderer" << std::endl;
		return false;
	}
	std::cout << "Renderer Created" << std::endl;

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
	if (texture == nullptr) {
		std::cout << "Failed to create texture" << std::endl;
		return false;
	}

	std::cout << "Texture Created" << std::endl;
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer2_Init(renderer);
	return true;
}

void selectGame() {
	nfdchar_t* outPath = NULL;
	nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);

	if (result == NFD_OKAY) {
		puts("Success!");
		puts(outPath);
		free(outPath);
	}
	else if (result == NFD_CANCEL) 
		puts("User pressed cancel.");
	else 
		printf("Error: %s\n", NFD_GetError());
	

	if (outPath != NULL)
		gameboy->loadGame(outPath);
}

void createPaletteWindow() {
	if (!isPaletteWindowOpen)
		return;

	ImGui::Begin("Palette");
	Color* colors = gameboy->getColors();
	ImGui::ColorEdit3("Color 1", &(*colors).r);
	ImGui::ColorEdit3("Color 2", &(*(colors + 1)).r);
	ImGui::ColorEdit3("Color 3", &(*(colors + 2)).r);
	ImGui::ColorEdit3("Color 4", &(*(colors + 3)).r);
}

void createControlWindow() {
	if (!isControlWindowOpen)
		return;

}

void createMainMenuBar() {
	ImGui::BeginMainMenuBar();

	if (ImGui::MenuItem("Open")) 
		selectGame();

	if (ImGui::MenuItem("Palettes"))
		isPaletteWindowOpen = !isPaletteWindowOpen;

	
		
	ImGui::EndMainMenuBar();
}


void createGUI() {
	createMainMenuBar();
	createPaletteWindow();
	createControlWindow();
}


void render(void const* buffer, int pitch) {
	SDL_UpdateTexture(texture, nullptr, buffer, pitch);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, nullptr, nullptr);
	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
	SDL_RenderPresent(renderer);
}


void input() {
	SDL_Event e;
	while (SDL_PollEvent(&e) > 0) {
		ImGui_ImplSDL2_ProcessEvent(&e);
		if (e.type == SDL_QUIT)
			running = false;

		// Process keydown events
		if (e.type == SDL_KEYDOWN) {
			if (e.key.keysym.sym == SDLK_ESCAPE)
				running = false;

			if (keyMap.count(e.key.keysym.sym)) 
				gameboy->pressButton(keyMap[e.key.keysym.sym]);
		}

		// Process keyup events
		if (e.type == SDL_KEYUP && keyMap.count(e.key.keysym.sym))
			gameboy->releaseButton(keyMap[e.key.keysym.sym]);

	}
}

void isUnloaded() {
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	createGUI();

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, nullptr, nullptr);
	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
	SDL_RenderPresent(renderer);
}


void run() {
	float time = 0;
	while (running) {

		input();
		gameboy->step();
		
		if (!gameboy->isRunning()) {
			isUnloaded();
			continue;
		}

		if (gameboy->canRender()) {
			ImGui_ImplSDLRenderer2_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();
			createGUI();
			ImGui::End();

			unsigned int* frame = gameboy->getFrame();
			int pitch = sizeof(frame[0]) * 160;
			render(frame, pitch);
		}
		
	}

	// Cleanup
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

bool init() {
	gameboy = new GameBoy();
	keyMap[SDLK_LEFT] = GAMEBOY_LEFT;
	keyMap[SDLK_RIGHT] = GAMEBOY_RIGHT;
	keyMap[SDLK_DOWN] = GAMEBOY_DOWN;
	keyMap[SDLK_UP] = GAMEBOY_UP;

	keyMap[SDLK_z] = GAMEBOY_B;
	keyMap[SDLK_x] = GAMEBOY_A; 
	keyMap[SDLK_c] = GAMEBOY_START;
	keyMap[SDLK_v] = GAMEBOY_SELECT;

	if (!initWindow()) {
		std::cout << "Failed to initialize window" << std::endl;
		return false;
	}

	return true;
}

int main(int argc, char* args[]) {
	if (!init())
		return -1;

	//load();
	run();

	delete gameboy;
	return 0;
}