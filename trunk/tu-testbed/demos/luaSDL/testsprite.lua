-- testsprite.lua

-- Lua conversion of the testsprite.c program from the SDL 1.2.2 distribution.


dofile("shadow40.lua")	-- helpers for structure access
dofile("luaSDL.lua")	-- load the wrappers for structure access.


NUM_SPRITES = 100
MAX_SPEED = 1

sprite = nil;
numsprites = NUM_SPRITES;
sprite_rects = {};
positions = {};
velocities = {};
sprites_visible = nil;
temp_rect = new_SDL_Rect();


function SDL_LoadBMP(file)
	return SDL_LoadBMP_RW(SDL_RWFromFile(file, "rb"), 1)
end


SDL_BlitSurface = SDL_UpperBlit;


function LoadSprite(screen, file)
--	SDL_Surface *temp;
	local temp;

	-- Load the sprite image
	sprite = SDL_LoadBMP(file);
	if sprite == nil then
		print("Couldn't load ", file, ": ", SDL_GetError());
		return (-1);
	end

	-- /* Set transparent pixel as the pixel at (0,0) */
	if sprite.format.palette ~= nil then
		SDL_SetColorKey(sprite, bit_or(SDL_SRCCOLORKEY, SDL_RLEACCEL), 0);
	end

	-- /* Convert sprite to video format */
	temp = SDL_DisplayFormat(sprite);
	SDL_FreeSurface(sprite);
	if temp == nil then
		print("Couldn't convert background: ", SDL_GetError());
		return(-1);
	end
	sprite = temp;

	-- /* We're ready to roll. :) */
	return(0);
end

function MoveSprites(screen, background)

	local i, nupdates;
	local position, velocity;

	nupdates = 0;
	-- /* Erase all the sprites if necessary */
	if sprites_visible == 1 then
		SDL_FillRect(screen, NULL, background);
	end

	-- /* Move the sprite, bounce at the wall, and draw */
	for i = 1, numsprites do
		position = positions[i];
		velocity = velocities[i];
		position.x = position.x + velocity.x;
		if position.x < 0 or position.x >= screen.w then
			velocity.x = -velocity.x;
			position.x = position.x + velocity.x;
		end
		position.y = position.y + velocity.y;
		if position.y < 0 or position.y >= screen.h then
			velocity.y = -velocity.y;
			position.y = position.y + velocity.y;
		end

		-- /* Blit the sprite onto the screen */
		temp_rect.x = position.x;
		temp_rect.y = position.y;
		temp_rect.w = position.w;
		temp_rect.h = position.h;
		SDL_BlitSurface(sprite, NULL, screen, temp_rect);
		sprite_rects[nupdates] = area;
		nupdates = nupdates + 1
	end

	-- /* Update the screen! */
	if bit_and(screen.flags, SDL_DOUBLEBUF) == SDL_DOUBLEBUF then
		SDL_Flip(screen);
	else
--		SDL_UpdateRects(screen, nupdates, sprite_rects);
		SDL_UpdateRect(screen, 0, 0, 0, 0)
	end
	sprites_visible = 1;
end


-- /* This is a way of telling whether or not to use hardware surfaces */
function FastestFlags(flags, width, height, bpp)
_ = [[
	const SDL_VideoInfo *info;

	/* Hardware acceleration is only used in fullscreen mode */
	flags = bit_or(flags, SDL_FULLSCREEN);

	/* Check for various video capabilities */
	info = SDL_GetVideoInfo();
	if ( info.blit_hw_CC && info.blit_fill ) {
		/* We use accelerated colorkeying and color filling */
		flags = bit_or(flags, SDL_HWSURFACE);
	}
	/* If we have enough video memory, and will use accelerated
	   blits directly to it, then use page flipping.
	 */
	if ( (flags & SDL_HWSURFACE) == SDL_HWSURFACE ) {
		/* Direct hardware blitting without double-buffering
		   causes really bad flickering.
		 */
		if ( info.video_mem*1024 > (height*width*bpp/8) ) {
			flags |= SDL_DOUBLEBUF;
		} else {
			flags &= ~SDL_HWSURFACE;
		}
	}

	/* Return the flags */
]]; _ = nil;
	return(flags);
end


function main_program(argv)
	local screen;
	local mem;
	local width, height;
	local video_bpp;
	local videoflags;
	local background;
	local i, done;
	local begin_time, frames;

	-- /* Initialize SDL */
	if SDL_Init(SDL_INIT_VIDEO) < 0 then
		print("Couldn't initialize SDL: ",SDL_GetError());
		exit(1);
	end
--	atexit(SDL_Quit);

	videoflags = bit_or(SDL_SWSURFACE, SDL_ANYFORMAT);
	width = 640;
	height = 480;
	video_bpp = 8;
	argv = argv or {}
	for i = 1, getn(argv) do
		if argv[i] == "-width" then
			width = tonumber(argv[i+1]);
			i = i + 1
		elseif argv[i] == "-height" then
			height = tonumber(argv[i+1]);
			i = i + 1
_ = [[
		elseif
		if ( strcmp(argv[argc-1], "-bpp") == 0 ) {
			video_bpp = atoi(argv[argc]);
			videoflags = bit_and(videoflags, ~SDL_ANYFORMAT);
			--argc;
		} else
		if ( strcmp(argv[argc], "-fast") == 0 ) {
			videoflags = FastestFlags(videoflags, width, height, video_bpp);
		} else
		if ( strcmp(argv[argc], "-hw") == 0 ) {
			videoflags ^= SDL_HWSURFACE;
		} else
		if ( strcmp(argv[argc], "-flip") == 0 ) {
			videoflags ^= SDL_DOUBLEBUF;
		} else
		if ( strcmp(argv[argc], "-fullscreen") == 0 ) {
			videoflags ^= SDL_FULLSCREEN;
		} else
		if ( isdigit(argv[argc][0]) ) {
			numsprites = atoi(argv[argc]);
]]; _ = nil;
		else
			print("Usage: main_program [-bpp N] [-hw] [-flip] [-fast] [-fullscreen] [numsprites]");
			exit(1);
		end
	end

	-- /* Set video mode */
	screen = SDL_SetVideoMode(width, height, video_bpp, videoflags);
	if screen == nil then
		print("Couldn't set ", width, "x", height, " video mode: ", SDL_GetError());
		exit(2);
	end

	-- /* Load the sprite */
	if LoadSprite(screen, "icon.bmp") < 0 then
		exit(1);
	end

	-- /* Allocate memory for the sprite info */
--	mem = (Uint8 *)malloc(4*sizeof(SDL_Rect)*numsprites);
--	if ( mem == NULL ) {
--		SDL_FreeSurface(sprite);
--		fprintf(stderr, "Out of memory!\n");
--		exit(2);
--	}
--	sprite_rects = (SDL_Rect *)mem;
--	positions = sprite_rects;
--	sprite_rects += numsprites;
--	velocities = sprite_rects;
--	sprite_rects += numsprites;
--	srand(time(NULL));

	for i = 1, numsprites do
		positions[i] = {}
		positions[i].x = random() * screen.w;
		positions[i].y = random() * screen.h;
		positions[i].w = sprite.w;
		positions[i].h = sprite.h;
		velocities[i] = {}
		velocities[i].x = 0;
		velocities[i].y = 0;
		while velocities[i].x == 0 and velocities[i].y == 0 do
			velocities[i].x = (random() * 2 - 1) * MAX_SPEED;
			velocities[i].y = (random() * 2 - 1) * MAX_SPEED;
		end
	end
	background = SDL_MapRGB(screen.format, 0, 0, 0);

	-- /* Print out information about our surfaces */
	print("Screen is at ", screen.format.BitsPerPixel, " bits per pixel");
	if bit_and(screen.flags, SDL_HWSURFACE) == SDL_HWSURFACE then
		print("Screen is in video memory");
	else
		print("Screen is in system memory");
	end
	if bit_and(screen.flags, SDL_DOUBLEBUF) == SDL_DOUBLEBUF then
		print("Screen has double-buffering enabled");
	end
	if bit_and(sprite.flags, SDL_HWSURFACE) == SDL_HWSURFACE then
		print("Sprite is in video memory");
	else
		print("Sprite is in system memory");
	end
	-- /* Run a sample blit to trigger blit acceleration */
_ = [[
	{ SDL_Rect dst;
		dst.x = 0;
		dst.y = 0;
		dst.w = sprite.w;
		dst.h = sprite.h;
		SDL_BlitSurface(sprite, NULL, screen, &dst);
		SDL_FillRect(screen, &dst, background);
	}
	if ( (sprite.flags & SDL_HWACCEL) == SDL_HWACCEL ) {
		printf("Sprite blit uses hardware acceleration\n");
	}
	if ( (sprite.flags & SDL_RLEACCEL) == SDL_RLEACCEL ) {
		printf("Sprite blit uses RLE acceleration\n");
	}
]]; _ = nil;

	-- /* Loop, blitting sprites and waiting for a keystroke */
	frames = 0;
	begin_time = SDL_GetTicks();
	done = 0;
	sprites_visible = 0;
	local event = new_SDL_Event();
	while done == 0 do
		-- /* Check for events */
		frames = frames + 1
		while SDL_PollEvent(event) ~= 0 do
			if event.type == SDL_KEYDOWN then
				-- /* Any keypress quits the app... */
				done = 1;
			elseif event.type == SDL_QUIT then
				done = 1;
			end
		end
		MoveSprites(screen, background);
	end
	delete_SDL_Event(event);
	SDL_FreeSurface(sprite);
--	free(mem);

	-- /* Print out some timing information */
	current_time = SDL_GetTicks();
	if current_time > begin_time then
		print((frames * 1000) / (current_time - begin_time), " frames per second");
	end

	SDL_Quit();

	return(0);
end


-- run the main program.
main_program()
