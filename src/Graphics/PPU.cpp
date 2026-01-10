#include "PPU.h"


PPU::PPU(PixelBuffer& buffer, Memory& memory, Interrupts& interrupts) : buffer(buffer), memory(memory), interrupts(interrupts), mode(OAMSCAN) {}

void PPU::step(int cycles) {
	this->cycles += cycles;

	//State machine that controls the mode the PPU is in
	switch (mode) {
		case OAMSCAN:
			OAMScan();
			break;

		case DRAWING:
			drawing();
			break;

		case HBLANK:
			hBlank();
			break;

		case VBlank:
			vBlank();
			break;
	}
}


//For 80 cycles, the PPU scans through memory and puts all objects that follow certain conditions into a buffer
//The sprites in the buffer are then rendered in the drawing state
//However since the rendering functions themselves filter out the invalid sprites, so this phase essentially does nothing
void PPU::OAMScan() {
	if (cycles < 80)
		return;
	cycles -= 80;

	//Switch to Drawing mode (Sets first 2 bits of LCD Status register to mode)
	unsigned char prev = memory.readByte(0xFF41) & 0xFC;
	memory.writeByte(0xFF41, prev | 3);
	this->mode = DRAWING;
}

//For 172 cycles, the PPU draws all layers for one row/scanline
void PPU::drawing() {
	if (cycles < 172)
		return;
	
	cycles -= 172;

	//Render current row
	renderScanline();

	//Switch to HBlank mode
	unsigned char prev = memory.readByte(0xFF41) & 0xFC;
	memory.writeByte(0xFF41, prev | 0);
	this->mode = HBLANK;

	//Initiate LCD interrupt if HBlank mode condition is true
	bool interrupt = (memory.readByte(0xFF41) >> 3) & 1;
	if (interrupt)
		interrupts.setInterruptFlag(LCD, true);

	//Checks if LY register (current scanline number) is equal to LYC register
	bool lyCoincidence = memory.readByte(0xFF44) == memory.readByte(0xFF45) + 1;
	bool lyEnable = (memory.readByte(0xFF41) >> 6) & 1;
				
	if (lyCoincidence && lyEnable) 
		interrupts.setInterruptFlag(LCD, true);
	memory.writeByte(0xFF41, memory.readByte(0xFF41) | (1 << 2));
}

//After a line finishes drawing, the PPU essentially pauses for 376 cycles (In actual hardware, the PPU waits until all pixels are transfers to the LCD)
//After this pause, if the scanline is currently at row 144, then we go to the VBlank state, otherwise we restart the process
void PPU::hBlank() {
	if (cycles < 204)
		return;

	cycles -= 204;
	unsigned char prev = memory.readByte(0xFF41) & 0xFC;

	//If current scanline is at row 144, then switch to VBlank phase
	if (memory.readByte(0xFF44) == 144) {
		canRender = true;
		memory.writeByte(0xFF41, prev | 1);
		this->mode = VBlank;
		interrupts.setInterruptFlag(VBLANK, true);
	}

	//If current scanline is not in row 144, then we switch back to the OAM scan state
	else {
		memory.writeByte(0xFF41, prev | 3);
		this->mode = OAMSCAN;
	}
}


//After the entire screen is drawn, the PPU waits for 10 scanlines (4560 cycles) until the next frame 
void PPU::vBlank() {
	canRender = false;
	if (cycles < 172)
		return;

	cycles -= 172;
	memory.writeByte(0xFF44, memory.readByte(0xFF44) + 1);

	//Once scanline reaches 155, exit VBlank
	if (memory.readByte(0xFF44) == 153) {
		//Reset screen and scanline register
		memory.writeByte(0xFF44, 0);
		//buffer.reset();

		//Change to OAM Scan mode
		unsigned char prev = memory.readByte(0xFF41) & 0xFC;
		memory.writeByte(0xFF41, prev | 2);
		this->mode = OAMSCAN;
	}
}

//Renders all 3 layers, background, window, and object, for one scanline, specified by the LY register in memory
//The LY register is then incremented, which represents it going to the next scanline
void PPU::renderScanline() {
	unsigned char y = memory.readByte(0xFF44);

	//If 0th bit in LCD display shader is 0, then the background and window layers are not rendered and replaced with white
	bool bgwEnable = memory.readByte(0xFF40) & 1;
	bool windowEnable = (memory.readByte(0xFF40) >> 5) & 1;
	bool spriteEnable = (memory.readByte(0xFF40) >> 1) & 1;

	if (bgwEnable)
		renderBackground(y);
	
	if (windowEnable && bgwEnable)
		renderWindow(y);

	if (spriteEnable)
		renderSprite(y);

	memory.writeByte(0xFF44, y + 1);
}


void PPU::renderBackground(unsigned char y) {
	//Controls which BG map to use
	unsigned short bgOffset = ((memory.readByte(0xFF40) >> 3) & 1) ? 0x9C00 : 0x9800;

	//Chooses which addressing mode to use for the window and background
	bool wOffset = ((memory.readByte(0xFF40) >> 4) & 1);

	//Get background tile Y position
	unsigned char scrollingY = memory.readByte(0xFF42);
	unsigned char tileY = ((y + scrollingY) / 8) % 32;

	//Iterate through all pixels in current scanline
	for (int x = 0; x < 160; ++x) {

		//From the current pixel in the row, get the specific 8x8 tile
		unsigned char scrollingX = memory.readByte(0xFF43);
		unsigned char tileX = ((x + scrollingX) / 8) % 32;

		//Retrieving which tile to render at background tile i
		unsigned char id = memory.readByte(bgOffset + ((tileY * 32) + tileX));

		//Get address of specfic tile in either $8000 unsigned or $8800 signed method
		unsigned short tileAddress = wOffset ? 0x8000 + (id * 16) : 0x8800 + ((char(id) + 128) * 16);
		unsigned short yIndex = 2 * ((y + scrollingY) % 8);

		//Get high and low tile data from memory
		unsigned char low = memory.readByte(tileAddress + yIndex);
		unsigned char high = memory.readByte((unsigned short)(tileAddress + yIndex + 1));

		//Retrieve specific color bit
		unsigned char index = (7 - ((x + scrollingX) % 8));
		unsigned char lowCol = (low >> index) & 1;
		unsigned char highCol = (high >> index) & 1;
		
		unsigned char color = (highCol << 1) | lowCol;
		unsigned char mappedColor = (memory.readByte(0xFF47) >> (color * 2)) & 0x3;


		//Based on 2 bit color value, convert it to RGB struct by inputting it into palette map and retrieving color object
		this->buffer.setPixel(x, y, *palette[mappedColor]);
	}
}

void PPU::renderWindow(unsigned char y) {
	//Controls which BG map to use
	unsigned char windowY = y - memory.readByte(0xFF4A);

	if (y < memory.readByte(0xFF4A))
		return;

	unsigned short bgOffset = ((memory.readByte(0xFF40) >> 6) & 1) ? 0x9C00 : 0x9800;

	//Chooses which addressing mode to use for the window and background
	bool wOffset = ((memory.readByte(0xFF40) >> 4) & 1);

	//Get background tile Y position
	unsigned char tileY = (windowY / 8);

	//Iterate through all tiles in current scanline
	for (int x = 0; x < 160; ++x) {
		if (x < (memory.readByte(0xFF4B) - 7))
			continue;

		unsigned char windowX = x - (memory.readByte(0xFF4B) - 7);
			
		unsigned char tileX = (windowX / 8);

		//Retrieving which tile to render at background tile i
		unsigned char id = memory.readByte(bgOffset + ((tileY * 32) + tileX));
		
		//Get address of specfic tile in either $8000 unsigned or $8800 signed method
		unsigned short tileAddress = wOffset ? 0x8000 + (id * 16) : 0x8800 + ((char(id) + 128) * 16);
		unsigned short yIndex = 2 * (windowY % 8);

		//Get high and low tile data from memory
		unsigned char low = memory.readByte(tileAddress + yIndex);
		unsigned char high = memory.readByte(tileAddress + yIndex + 1);

		//Retrieve specific color bit
		unsigned char index = (7 - (windowX % 8));
		unsigned char lowCol = (low >> index) & 1;
		unsigned char highCol = (high >> index) & 1;

		unsigned char color = (highCol << 1) | lowCol;
		unsigned char mappedColor = (memory.readByte(0xFF47) >> (color * 2)) & 0x3;

		//Based on 2 bit color value, convert it to RGB struct by inputting it into palette map and retrieving color object
		this->buffer.setPixel(x, y, *palette[mappedColor]);
	}
}

void PPU::renderSprite(unsigned char y) {
	//Bit 2 of the LCD control register specifies the height format of the sprite tiles
	//If they are (8x16) tiles or (8x8) tiles
	unsigned char height = (memory.readByte(0xFF40) >> 2) & 1 ? 16 : 8;

	//Iterate through all sprites in OAM
	for (int i = 0xFE00; i <= 0xFE9F; i += 4) {
		//Get sprite data
		char yPosition = memory.readByte(i) - 16;
		char xPosition = memory.readByte(i + 1) - 8;
		unsigned char id = memory.readByte(i + 2);
		unsigned char flags = memory.readByte(i + 3);

		//If sprite doesn't intersect the scanline, then we don't render it
		if (y < yPosition || y >= yPosition + height) 
			continue;

		//Get specific address for the current row of the tile
		unsigned short tileAddress = 0x8000 + (id * 16);
		bool yFlip = (flags >> 6) & 1;
		unsigned short yIndex = yFlip ? 2 * ((height - 1) - (y - yPosition)) : 2 * (y - yPosition);
			
		//Get high and low tile data from memory for specific scanline		
		unsigned char low = memory.readByte(tileAddress + yIndex);
		unsigned char high = memory.readByte(tileAddress + yIndex + 1);
			
		bool xFlip = (flags >> 5) & 1;

		//Iterate through all pixels of sprite in current row/scanline
		for (int x = 0; x < 8; ++x) {
			int index = xFlip ? x : 7 - x;
			unsigned char lowCol = (low >> index) & 1;
			unsigned char highCol = (high >> index) & 1;

			bool priority = (flags >> 7) & 1;

			//Getting color that is already in the background
			Color other = buffer.getPixel(x + xPosition, y);
			int existingPixel = ((unsigned char)other.r << 16) | ((unsigned char)other.g << 8) | (unsigned char)other.b;

			//Getting the color for index 0
			int temp = (memory.readByte(0xFF47)) & 0x3;
			int tempColor = ((unsigned char)(*(palette[temp])).r << 16) | ((unsigned char)(*(palette[temp])).g << 8) | (unsigned char)(*(palette[temp])).b;

			//If Background/window priority flag is set and background color at pixel is not transparent, then we don't render pixel
			if (priority && existingPixel != tempColor)
				continue;

			unsigned char color = highCol << 1 | lowCol;

			unsigned short dmgPalette = !((flags >> 4) & 1) ? 0xFF48 : 0xFF49;
			unsigned char mappedColor = (memory.readByte(dmgPalette) >> (color * 2)) & 0x3;

			//Don't render if set to transparent color which is index 0
			if (color == 0)
				continue;

			//if (color != 0 && (!priority || existingPixel == 0xFFFFFF))
			this->buffer.setPixel(xPosition + x, y, *palette[mappedColor]);
		}
	}
}

void PPU::reset() {
	mode = OAMSCAN;
	cycles = 0;
	canRender = false;
}

Color* PPU::getColors()
{
	return colors;
}
