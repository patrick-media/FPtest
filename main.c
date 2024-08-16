#define _CRT_SECURE_NO_DEPRECATE // danger zone
#include<SDL.h>
#include<stdio.h>
#include<stdint.h>
#include<stdbool.h>

// error handling
#define ASSERT( _e, ... ) if( !( _e ) ) { fprintf( stderr, __VA_ARGS__ ); exit( 1 ); }

// screen variables
#define SCREEN_WIDTH 384
#define SCREEN_HEIGHT 216

// math constants
#define PI 3.14159265f
#define TAU ( 2.0f * PI )
#define PI_2 ( PI / 2.0f )
#define PI_4 ( PI / 4.0f )

// vector / general math functions
#define dot( v0, v1 ) ( v0.x * v1.x ) + ( v0.y * v1.y )
#define length( v ) sqrtf( dot( v, v ) )
#define normalize( u ) ( v2 ) { u.x / length( u ), u.y / length( u ) }
#define ifnan( _x, _alt ) ( isnan( _x ) ? ( _alt ) : _x )
#define min( a, b ) ( a < b ? a : b )
#define max( a, b ) ( a > b ? a : b )
#define clamp( x, _min, _max ) ( min( max ( x, _min ), _max ) )

// degrees / radians conversion
#define DEG2RAD( x ) ( x * ( PI / 180.0f ) )
#define RAD2DEG( x ) ( x * ( 180.0f / PI ) )

// FOV / eye height
#define HFOV DEG2RAD( 90.0f )
#define VFOV 0.5f
#define EYE_Z 1.65f

// near / far clipping planes
#define ZNEAR 0.0001f
#define ZFAR 128.0f

// vector2 types
typedef struct v2_s { float x, y; } v2;
typedef struct v2i_s { int x, y; } v2i;

// wall info struct
typedef struct wall_s {
	v2 p1,p2;
	int height,color;
} wall;

// window / character information
struct {
	SDL_Window *window;
	SDL_Texture *texture;
	SDL_Renderer *renderer;
	uint32_t pixels[ SCREEN_WIDTH * SCREEN_HEIGHT ];
	struct {
		v2 pos;
		float angle, anglecos, anglesin;
	} camera;
	wall walls[ 128 ];
	int nwalls;
	bool lerp_error;
	bool quit;
} state;

// linear interpolation
float lerp( int x0, int y0, int x1, int y1, int p ) {
	// original equation: y0 + [(p - x0)(y1 - y0)] / (x1 - x0)
	// find the y coordinate given a starting / ending point and
	// an x coordinate on the line.
	// this is just a convoluted return statement
	// contains ternary operators to eliminate division by zero, returns
	// -1 if that occurs.
	if( ( x1 - x0 ) != 0 ) {
		return ( float )( y0 + ( ( p - x0 ) * ( y1 - y0 ) ) / ( x1 - x0 ) );
	} else {
		/*
		printf( "lerp failed - div by zero\n" );
		printf( "x0 = %i\n", x0 );
		printf( "x1 = %i\n\n", x1 );
		*/
		state.lerp_error = true;
		return -1;
	}
}
void E_HANG( void ) {
	printf( "We are hanging here...\n" );
	while( true );
}
// rotate vector v by angle a
static inline v2 rotate( v2 v, float a ) {
	// simple matrix multiplication
	//  -> [ cos a   -sin a ]
	//  v  [ sin a    cos a ]
	return ( v2 ){
		( v.x * cos( a ) ) - ( v.y * sin( a ) ),
		( v.x * sin( a ) ) + ( v.y * cos( a ) )
	};
}
// convert point in relation to angle into a screen x coordinate
static inline int screen_angle_to_x( float angle ) {
	// no clue how this works, self explanatory from the above comment
	// but very convoluted reading this.
	return
		( ( int ) ( SCREEN_WIDTH / 2 ) )
		* ( 1.0f - tan( ( ( angle + ( HFOV / 2.0f ) ) / HFOV ) * PI_2 - PI_4 ) );
}
// world space -> camera space (origin)
static inline v2 world_pos_to_camera( v2 p ) {
	// vector u is the difference between the camera pos and the point
	const v2 u = { p.x - state.camera.pos.x, p.y - state.camera.pos.y };
	// rotate the point so that it is as if the camera is the origin.
	// another matrix multiplication
	//  -> [ u.x * sin cam - u.y * cos cam ]
	//  u  [ u.x * cos cam + u.y * sin cam ]
	return ( v2 ) {
		u.x * state.camera.anglesin - u.y * state.camera.anglecos,
			u.x * state.camera.anglecos + u.y * state.camera.anglesin
	};
}
// normalize angle to +/-PI
static inline float normalize_angle( float a ) {
	// not sure how this works internally, but it doesn't allow
	// returns greater than 3.14 and less than -3.14
	return a - ( TAU * floor( ( a + PI ) / TAU ) );
}
// intersection of two line segments
// returns (NAN, NAN) if there is none
static inline v2 intersect_segs( v2 a0, v2 a1, v2 b0, v2 b1 ) {
	// no clue how this works, not even gonna begin to try to explain it
	// i do know that it finds if lines a0->a1 and b0->b1 intersect
	// at any given point, and it returns that ordered pair.
	const float d =
		( ( a0.x - a1.x ) * ( b0.y - b1.y ) )
		- ( ( a0.y - a1.y ) * ( b0.x - b1.x ) );

	if( fabsf( d ) < 0.000001f ) { return ( v2 ){ NAN, NAN }; }

	const float
		t = ( ( ( a0.x - b0.x ) * ( b0.y - b1.y ) )
			- ( ( a0.y - b0.y ) * ( b0.x - b1.x ) ) ) / d,
		u = ( ( ( a0.x - b0.x ) * ( a0.y - a1.y ) )
			- ( ( a0.y - b0.y ) * ( a0.x - a1.x ) ) ) / d;

	return ( t >= 0 && t <= 1 && u >= 0 && u <= 1 ) ?
		( ( v2 ) {
		a0.x + ( t * ( a1.x - a0.x ) ),
			a0.y + ( t * ( a1.y - a0.y ) ) } )
		: ( ( v2 ){ NAN, NAN } );
}
// display a vertical line of pixels from (x, y0) to (x, y1)
void verline( int x, int y0, int y1, int color ) {
	// check if each coordinate is within the bounds of
	// the screen before we write memory we don't own
	if( x < 0 ) x = 0;
	if( x > SCREEN_WIDTH - 1 ) x = SCREEN_WIDTH - 1;
	if( y0 < 0 ) y0 = 0;
	if( y0 > SCREEN_HEIGHT - 1 ) y0 = SCREEN_HEIGHT - 1;
	if( y1 < 0 ) y1 = 0;
	if( y1 > SCREEN_HEIGHT - 1 ) y1 = SCREEN_HEIGHT - 1;
	// draw loop
	for( int y = y0; y < y1; y++ ) {
		state.pixels[ y * SCREEN_WIDTH + x ] = color;
	}
}

// draw a wall given a wall struct object
// struct contains 2 points, height, and color
// (top 2 points can be assumed from the height)
void WALLSCAN( wall w_wall ) {
	state.lerp_error = false;
	// clipping angles/distances?
	const v2
		zdl = rotate( ( ( v2 ){ 0.0f, 1.0f } ), +( HFOV / 2.0f ) ),
		zdr = rotate( ( ( v2 ){ 0.0f, 1.0f } ), -( HFOV / 2.0f ) ),
		znl = ( v2 ){ zdl.x * ZNEAR, zdl.y * ZNEAR }, // z near left
		znr = ( v2 ){ zdr.x * ZNEAR, zdr.y * ZNEAR }, // z near right
		zfl = ( v2 ){ zdl.x * ZFAR, zdl.y * ZFAR }, // z far left
		zfr = ( v2 ){ zdr.x * ZFAR, zdr.y * ZFAR }; // z far right
	// world pos 0 & 1
	v2
		wp0 = world_pos_to_camera( w_wall.p1 ),
		wp1 = world_pos_to_camera( w_wall.p2 );
	// both negative
	if( wp0.y <= 0 && wp1.y <= 0 ) {
		/* debug
		printf( "\n"
			"point1_v.y: %f\n"
			"point2_v.y: %f\n",
			wp0.y,
			wp1.y
		);
		printf( "failed 2\n\n" );
		*/
		return;
	}
	// normalized angles 0 & 1
	float
		nm0 = normalize_angle( atan2( wp0.y, wp0.x ) - PI_2 ),
		nm1 = normalize_angle( atan2( wp1.y, wp1.x ) - PI_2 );
	// check clipping bounds on FOV
	if( wp0.y < ZNEAR
		|| wp1.y < ZNEAR
		|| nm0 > +( HFOV / 2 )
		|| nm1 < -( HFOV / 2 ) ) {
		const v2
			il = intersect_segs( wp0, wp1, znl, zfl ),
			ir = intersect_segs( wp0, wp1, znr, zfr );
		// recalculate if nan
		if( !isnan( il.x ) ) {
			wp0 = il;
			nm0 = normalize_angle( atan2( wp0.y, wp0.x ) - PI_2 );
		}
		// recalculate if nan
		if( !isnan( ir.x ) ) {
			wp1 = ir;
			nm1 = normalize_angle( atan2( wp1.y, wp1.x ) - PI_2 );
		}
	}

	// don't know why this doesn't work, it doesn't display
	// properly if this is left in.
	/* debug
	printf( "3\n" );
	if( point1_n < point2_n ) {
		printf( "\n"
			"point1_n: %f\n"
			"point2_n: %f\n",
			point1_n,
			point2_n
		);
		printf( "failed 3\n\n" );
		return;
	}
	*/

	// forget the point if it's not within the view (HFOV since it's an x coord)
	if( ( nm0 < -( HFOV / 2 ) && nm1 < -( HFOV / 2 ) )
		|| ( nm0 > +( HFOV / 2 ) && nm1 > +( HFOV / 2 ) ) ) {
		/* debug
		printf( "\n"
			"HFOV / 2: %f\n"
			"point1_n: %f\n"
			"point2_n: %f\n",
			HFOV / 2,
			nm0,
			nm1
		);
		printf( "failed 4\n\n" );
		*/
		return;
	}
	// screen x 0 & 1
	int
		sx0 = screen_angle_to_x( nm0 ),
		sx1 = screen_angle_to_x( nm1 );
	// for some reason these values can be on the crazy
	// spectrum of signed integers like -2.147 b
	// if there's some kind of overflow we'll just set them to a
	// normal integer since it's causing freezes with the for loop later
	if( sx0 < 0 ) sx0 = -1;
	if( sx1 < 0 ) sx1 = -1;

	/* debug
	printf( 
		"sx0: %i\n"
		"sx1: %i\n\n",
		sx0,
		sx1
	);
	*/

	// this gets multiplied later, it must not be important /s
	const float
		sy0 = ifnan( ( VFOV * SCREEN_HEIGHT ) / wp0.y, 1e10 ),
		sy1 = ifnan( ( VFOV * SCREEN_HEIGHT ) / wp1.y, 1e10 );
	// floor / ceiling heights
	const int zfloor = 0, zceil = w_wall.height;
	// screen y coords for each corner of the polygonal wall
	const int
		yf0 = ( SCREEN_HEIGHT / 2 ) + ( int )( ( zfloor - EYE_Z ) * sy0 ),
		yf1 = ( SCREEN_HEIGHT / 2 ) + ( int )( ( zfloor - EYE_Z ) * sy1 ),
		yc0 = ( SCREEN_HEIGHT / 2 ) + ( int )( ( zceil - EYE_Z ) * sy0 ),
		yc1 = ( SCREEN_HEIGHT / 2 ) + ( int )( ( zceil - EYE_Z ) * sy1 );
	// simplified since the lerp functions were gross
	// also i don't know if the lerp function works the opposite way around
	// safest bet is smaller->bigger L->R in the params
	int
		//x_start = min( sx0, sx1 ),
		//x_end = max( sx0, sx1 );
		x_start = sx0,
		x_end = sx1;
	for( int i = 0; i <= x_end - x_start; i++ ) {
		// linearly interpolate the y coordinates for each
		// x coordinate between the starting and ending points of
		// the wall. also we know the very edge y coords, so just
		// interpolate it.
		int 
			floor_lerp = ( int )lerp( x_start, yf0, x_end, yf1, x_start + i ),
			ceil_lerp = ( int )lerp( x_start, yc0, x_end, yc1, x_start + i );
		// finally, draw a line.
		verline( min( sx0, sx1 ) + i,
			floor_lerp,
			ceil_lerp,
			w_wall.color );
	}
}
// manually pass arguments without casts in the params
void WALLSCAN_M( v2 p1, v2 p2, int height, int color ) {
	wall new_wall = { 0 };
	new_wall.p1 = p1;
	new_wall.p2 = p2;
	new_wall.height = height;
	new_wall.color = color;
	WALLSCAN( new_wall );
}

// load walls from a file
void map_file( char* path ) {
	FILE *f = fopen( path, "r" );
	if( f ) {
		char line[ 1024 ];
		while( fgets( line, sizeof( line ), f ) ) {
			wall *wall_queued = &state.walls[ state.nwalls++ ];
			const char *p = line;
			while( isspace( *p ) ) p++;
			if( !*p || *p == '#' ) continue;
			if( sscanf(
				p,
				"%f %f %f %f %d %x",
				&wall_queued->p1.x,
				&wall_queued->p1.y,
				&wall_queued->p2.x,
				&wall_queued->p2.y,
				&wall_queued->height,
				&wall_queued->color
			) != 6 ) {
				printf( "sscanf returned %i\n", sscanf( p, "%f %f %f %f %i %x",
					&wall_queued->p1.x,
					&wall_queued->p1.y,
					&wall_queued->p2.x,
					&wall_queued->p2.y,
					&wall_queued->height,
					&wall_queued->color ) );
				printf( "%f\n%f\n%f\n%f\n%i\n0x%X\n\n",
					wall_queued->p1.x,
					wall_queued->p1.y,
					wall_queued->p2.x,
					wall_queued->p2.y,
					wall_queued->height,
					wall_queued->color
					);
				printf( "Error processing wall\n" );
				E_HANG();
			}
		}
		fclose( f );
	}
}

wall wall1 = { { 5.0f, 5.0f }, { 5.0f, 10.0f }, 7, 0xFFFFFFFF };
wall wall2 = { { 5.0f, 10.0f }, { 10.0f, 15.0f }, 7, 0xFFFF00FF };
wall wall3 = { { 10.0f, 15.0f }, { 15.0f, 15.0f }, 7, 0xFFFFFF00 };
wall wall4 = { { 15.0f, 15.0f }, { 20.0f, 10.0f }, 7, 0xFF00FFFF };
wall wall5 = { { 20.0f, 10.0f }, { 20.0f, 5.0f }, 7, 0xFFFFFFFF };
wall wall6 = { { 20.0f, 5.0f }, { 15.0f, 0.0f }, 7, 0xFFFF00FF };
wall wall7 = { { 15.0f, 0.0f }, { 10.0f, 0.0f }, 7, 0xFFFFFF00 };
wall wall8 = { { 10.0f, 0.0f }, { 5.0f, 5.0f }, 7, 0xFF00FFFF };

wall wall9 = { { 5.0f, 15.0f }, { 10.0f, 15.0f }, 7, 0xFFFFFFFF };
wall wall10 = { { 10.0f, 15.0f }, { 10.0f, 20.0f }, 7, 0xFFFF00FF };
void render( void ) {
	/*
	WALLSCAN( wall1 );
	WALLSCAN( wall2 );
	WALLSCAN( wall3 );
	WALLSCAN( wall4 );
	WALLSCAN( wall5 );
	WALLSCAN( wall6 );
	WALLSCAN( wall7 );
	WALLSCAN( wall8 );
	*/
	/*
	WALLSCAN( wall9 );
	WALLSCAN( wall10 );
	*/
	//WALLSCAN_F( "E:\\Patrick\\4_repos\\FPtest\\FPtest\\level1.txt" );
	for( int x = 0; x < SCREEN_WIDTH; x++ ) {
		verline( x, SCREEN_HEIGHT / 2, SCREEN_HEIGHT - 1, 0xFF757575 );
	}
	for( int x = 0; x < SCREEN_WIDTH; x++ ) {
		verline( x, 0, SCREEN_HEIGHT / 2, 0xFF424242 );
	}
	for( int i = 0; i < state.nwalls; i++ ) {
		WALLSCAN( state.walls[ i ] );
	}
}
int main( int argc, char* argv[] ) {
	// initialize (assert for error checking, will stop application and
	// print to standard error file if there is an error)
	ASSERT( !SDL_Init( SDL_INIT_VIDEO ), "SDL failed to initialize: %s\n", SDL_GetError() );
	// create window
	state.window = SDL_CreateWindow(
		"DEMO",
		SDL_WINDOWPOS_CENTERED_DISPLAY( 0 ),
		SDL_WINDOWPOS_CENTERED_DISPLAY( 0 ),
		1280,
		720,
		SDL_WINDOW_ALLOW_HIGHDPI );
	// error check
	ASSERT( state.window, "failed to create SDL window: %s\n", SDL_GetError() );
	// create renderer - we need this to put pixels in places manually
	// the pixels buffer will get replaced as the texture for the entire
	// window every frame
	state.renderer = SDL_CreateRenderer( state.window, -1, SDL_RENDERER_PRESENTVSYNC );
	// error check
	ASSERT( state.renderer, "failed to create SDL renderer: %s\n", SDL_GetError() );
	// create texture - pixels buffer gets set as the texture
	state.texture = SDL_CreateTexture(
		state.renderer,
		SDL_PIXELFORMAT_ABGR8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH,
		SCREEN_HEIGHT );
	// error check
	ASSERT( state.texture, "failed to create SDL texture: %s\n", SDL_GetError() );

	state.nwalls = 0;
	map_file( "E:\\Patrick\\3_repos\\FPtest\\FPtest\\level3.txt" );

	// initialize camera
	state.camera.pos = ( v2 ) { 10.0f, 10.0f };

	// core loop
	while( !state.quit ) {
		// SDL events (messaging system like win32)
		SDL_Event ev;
		while( SDL_PollEvent( &ev ) ) {
			switch( ev.type ) {
			case SDL_QUIT:
				state.quit = true;
				break;
			}
		}

		// rotation speed & movement speed
		float rot_speed = 3.0f * 0.016f, move_speed = 3.0f * 0.016f;
		
		// keyboard information
		const uint8_t *keystate = SDL_GetKeyboardState( NULL );
		// left arrow key (look left)
		if( keystate[ SDLK_LEFT & 0xFFFF ] ) {
			state.camera.angle += rot_speed;
		}
		// right arrow key (look right)
		if( keystate[ SDLK_RIGHT & 0xFFFF ] ) {
			state.camera.angle -= rot_speed;
		}
		// variables so we don't calculate each (slow) function
		// every time they are referenced (there are a lot)
		state.camera.anglecos = cos( state.camera.angle );
		state.camera.anglesin = sin( state.camera.angle );
		// sideways strafe - just anglecos/anglesin +/- PI_2 for strafing angles
		// same reason for separate variables
		float
			p_sanglecos = cos( state.camera.angle + PI_2 ),
			n_sanglecos = cos( state.camera.angle - PI_2 ),
			p_sanglesin = sin( state.camera.angle + PI_2 ),
			n_sanglesin = sin( state.camera.angle - PI_2 );
		// left shift (sprint)
		if( keystate[ SDLK_LSHIFT & 0xFFFF ] ) {
			move_speed *= 7.0f;
		}
		// up arrow (move forward)
		if( keystate[ SDLK_UP & 0xFFFF ] ) {
			state.camera.pos = ( v2 ){
				state.camera.pos.x + ( move_speed * state.camera.anglecos ),
				state.camera.pos.y + ( move_speed * state.camera.anglesin )
			};
		}
		// down arrow (move backward)
		if( keystate[ SDLK_DOWN & 0xFFFF ] ) {
			state.camera.pos = ( v2 ){
				state.camera.pos.x - ( move_speed * state.camera.anglecos ),
				state.camera.pos.y - ( move_speed * state.camera.anglesin )
			};
		}
		// w (move forward)
		if( keystate[ SDL_SCANCODE_W ] ) {
			state.camera.pos = ( v2 ){
				state.camera.pos.x + ( move_speed * state.camera.anglecos ),
				state.camera.pos.y + ( move_speed * state.camera.anglesin )
			};
		}
		// s (move backward)
		if( keystate[ SDL_SCANCODE_S ] ) {
			state.camera.pos = ( v2 ){
				state.camera.pos.x - ( move_speed * state.camera.anglecos ),
				state.camera.pos.y - ( move_speed * state.camera.anglesin )
			};
		}
		// a (move left)
		if( keystate[ SDL_SCANCODE_A ] ) {
			state.camera.pos = ( v2 ){
				state.camera.pos.x + ( move_speed * p_sanglecos ),
				state.camera.pos.y + ( move_speed * p_sanglesin )
					};
		}
		// d (move right)
		if( keystate[ SDL_SCANCODE_D ] ) {
			state.camera.pos = ( v2 ){
				state.camera.pos.x + ( move_speed * n_sanglecos ),
				state.camera.pos.y + ( move_speed * n_sanglesin )
			};
		}
		// clear pixels buffer
		memset( state.pixels, 0, sizeof( state.pixels ) );
		// render
		render();

		// debug
		printf( "x: %f\n"
			"y: %f\n"
			"angle RAD: %f\n"
			"angle DEG: %f\n\n",
			state.camera.pos.x,
			state.camera.pos.y,
			state.camera.angle,
			RAD2DEG( state.camera.angle )
		);
		
		// set the texture to be the pixels buffer
		SDL_UpdateTexture( state.texture, NULL, state.pixels, SCREEN_WIDTH * 4 );
		// update the renderer buffer?
		SDL_RenderCopyEx( state.renderer, state.texture, NULL, NULL, 0.0, NULL, SDL_FLIP_VERTICAL );
		// replace the previous frame with the current frame
		SDL_RenderPresent( state.renderer );
	}
	// free memory so we don't have leaks
	SDL_DestroyTexture( state.texture );
	SDL_DestroyRenderer( state.renderer );
	SDL_DestroyWindow( state.window );
	return 0;
}