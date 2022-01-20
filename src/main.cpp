/*This source code copyrighted by Lazy Foo' Productions (2004-2020)
and may not be redistributed without written permission.*/
#include "cpu.hpp"
//Using SDL and standard IO
#include <SDL2/SDL.h>
#include <stdio.h>
#include <iostream>
#include <array>
#include <fstream>
//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
std::ofstream uwu("debug.out");
uint8_t *update_gpu(CPU &cpu)
{
	uint32_t status_reg = cpu.mmu->read32(0xFFFFFFFFA4400000);
	uint32_t ptr = cpu.mmu->read32(0xFFFFFFFFA4400004) & 0b1111'1111'1111'1111'1111'1111;
	const uint32_t width = cpu.mmu->read32(0xFFFFFFFFA4400008);
	const uint32_t scale = cpu.mmu->read32(0xFFFFFFFFA4400034);
	uint8_t format = status_reg & 0b11; //0 RGBA5553 : 1 RGBA8888

	uint32_t height = (15 * (scale)) / 64;
	uint8_t *new_pixels = new uint8_t[width * height * 8];
	const uint64_t addr = ptr + 0xFFFF'FFFF'8000'0000;
	for (int x = 0; x < width * 4; x++)
	{
		for (int y = 0; y < height * 4; y++)
		{
			//new_pixels[x + width * y] = cpu.mmu->read32(addr + 4 * (x + width * y));
			uint8_t pixel = cpu.mmu->read8(addr + x * height + y);
			new_pixels[x * height + y] = pixel;
			// new_pixels[4 * x + 4 * y * width + 2] = cpu.mmu->read8(ptr + 0xFFFFFFFF00000000 + 4 * x + 4 * y * width + 2);
			// new_pixels[4 * x + 4 * y * width + 3] = cpu.mmu->read8(ptr + 0xFFFFFFFF00000000 + 4 * x + 4 * y * width + 3);
			// uwu << (int)cpu.mmu->read8(ptr + x * height + y) << ' ';
		}
		//uwu << "\n\n\n\n\n";
	}
	//std::cout << std::hex << ptr << ' ' << width << ' ' << height << ' ' << (int)format << '\n';

	//exit(0);
	if (width || height)
		return new_pixels;
	else
		return nullptr;
}

int main(int argc, char *args[])
{
	//The window we'll be rendering to
	SDL_Window *window = NULL;
	SDL_Texture *texture = NULL;
	SDL_Renderer *renderer = NULL;
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}
	else
	{
		//Create window
		window = SDL_CreateWindow("Hayami", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (window == NULL)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		}
		else
		{
			CPU cpu;
			int fake = 0;
			SDL_Event e;
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
			while (true)
			{
				const uint32_t width = cpu.mmu->read32(0xFFFFFFFFA4400008);
				const uint32_t scale = cpu.mmu->read32(0xFFFFFFFFA4400034);
				uint32_t height = (15 * (scale)) / 64;
				fake++;

				if (fake == 6000)
				{
					uint8_t *new_pixels = update_gpu(cpu);

					texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height);

					fake = 0;
					SDL_UpdateTexture(texture, NULL, new_pixels, width * 4);
					SDL_RenderClear(renderer);
					SDL_RenderCopy(renderer, texture, NULL, NULL);
					SDL_RenderPresent(renderer);

					SDL_DestroyTexture(texture);
				}

				SDL_PollEvent(&e);
				if (e.type == SDL_QUIT)
				{
					SDL_Log("Program quit after %i ticks", e.quit.timestamp);
					break;
				}

				cpu.emulate_cycle(cpu.pc);
				cpu.pc += 4;
			}
		}
	}

	//Destroy window
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	//Quit SDL subsystems
	SDL_Quit();

	return 0;
}
