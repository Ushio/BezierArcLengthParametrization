#include "pr.hpp"
#include <iostream>
#include <memory>

float sqr( float x )
{
	return x * x;
}

template <class T>
T bezier( T ps[4], float t )
{
	const auto p4 = glm::mix( ps[0], ps[1], t );
	const auto p5 = glm::mix( ps[1], ps[2], t );
	const auto p6 = glm::mix( ps[2], ps[3], t );

	const auto p7 = glm::mix( p4, p5, t );
	const auto p8 = glm::mix( p5, p6, t );

	return glm::mix( p7, p8, t );
}
template <class T>
T dbezier( T ps[4], float t )
{
	return 3.0f * sqr( 1.0f - t ) * ( ps[1] - ps[0] ) +
		   6.0f * t * ( 1.0f - t ) * ( ps[2] - ps[1] ) +
		   3.0f * sqr( t ) * ( ps[3] - ps[2] );
}

template <class F>
float gaussianQuadratureN3( F f, float a, float b )
{
	const float b_minus_a_over_2 = ( b - a ) * 0.5f;
	const float b_plus_a_over_2 = ( b + a ) * 0.5f;

	struct
	{
		float w;
		float x;
	} wx[3] = {
		0.8888888888888888,
		0.0,
		0.5555555555555556,
		-0.7745966692414834,
		0.5555555555555556,
		0.7745966692414834,
	};
	float sum = 0.0f;
	for ( int i = 0; i < sizeof( wx ) / sizeof( wx[0] ); ++i )
	{
		float x = std::fma( wx[i].x, b_minus_a_over_2, b_plus_a_over_2 );
		sum += wx[i].w * f( x );
	}
	return sum * b_minus_a_over_2;
}
template <class F>
float gaussianQuadratureN10( F f, float a, float b )
{
	const float b_minus_a_over_2 = ( b - a ) * 0.5f;
	const float b_plus_a_over_2 = ( b + a ) * 0.5f;

	struct
	{
		float w;
		float x;
	} wx[10] = {
		0.2955242247147529,
		-0.1488743389816312,
		0.2955242247147529,
		0.1488743389816312,
		0.2692667193099963,
		-0.4333953941292472,
		0.2692667193099963,
		0.4333953941292472,
		0.2190863625159820,
		-0.6794095682990244,
		0.2190863625159820,
		0.6794095682990244,
		0.1494513491505806,
		-0.8650633666889845,
		0.1494513491505806,
		0.8650633666889845,
		0.0666713443086881,
		-0.9739065285171717,
		0.0666713443086881,
		0.9739065285171717,
	};
	float sum = 0.0f;
	for ( int i = 0; i < sizeof( wx ) / sizeof( wx[0] ); ++i )
	{
		float x = std::fma( wx[i].x, b_minus_a_over_2, b_plus_a_over_2 );
		sum += wx[i].w * f( x );
	}
	return sum * b_minus_a_over_2;
}

// [0 - b]
float bezierLengthN3( glm::vec3 ps[4], float b )
{
	return gaussianQuadratureN3(
		[&]( float t ) { return glm::length( dbezier( ps, t ) ); },
		0.0f,
		b );
}

// [0 - b]
float bezierLengthN10( glm::vec3 ps[4], float b )
{
	return gaussianQuadratureN10(
		[&]( float t ) { return glm::length( dbezier( ps, t ) ); },
		0.0f,
		b );
}

int main()
{
	using namespace pr;

	Config config;
	config.ScreenWidth = 1920;
	config.ScreenHeight = 1080;
	config.SwapInterval = 1;
	Initialize( config );

	Camera3D camera;
	camera.origin = {10, 0, 0};
	camera.lookat = {0, 0, 0};
	camera.perspective = 0;
	camera.zUp = false;

	double e = GetElapsedTime();

	while ( pr::NextFrame() == false )
	{
		if ( IsImGuiUsingMouse() == false )
		{
			UpdateCameraBlenderLike( &camera );
		}

		ClearBackground( 0.1f, 0.1f, 0.1f, 1 );

		BeginCamera( camera );

		PushGraphicState();

		DrawGrid( GridAxis::XZ, 1.0f, 10, {128, 128, 128} );
		DrawXYZAxis( 1.0f );

		static float rs[2] = {0, 0.5f};
		static glm::vec3 ps[4] = {
			{0, 0, 0},
			{0, 0.1, 0},
			{0, 3.9, 0},
			{0, 4, 0},
		};
		//static glm::vec3 ps[4] = {
		//	{0, 0, 0},
		//	{0, 0.1, 0},
		//	{2.5, 1, 0},
		//	{4, 0, 0},
		//};
		ManipulatePosition( camera, &ps[0], 0.3f );
		ManipulatePosition( camera, &ps[1], 0.3f );
		ManipulatePosition( camera, &ps[2], 0.3f );
		ManipulatePosition( camera, &ps[3], 0.3f );

		// Arc length parametarization
		float s1 = bezierLengthN10( ps, 1.0f / 3.0f ) / bezierLengthN10( ps, 1.0f );
		float s2 = bezierLengthN10( ps, 2.0f / 3.0f ) / bezierLengthN10( ps, 1.0f );
		float vs[] = {
			0.0f,
			( 18.0f * s1 - 9.0f * s2 + 2.0f ) / 6.0f,
			( -9.0f * s1 + 18.0f * s2 - 5.0f ) / 6.0f,
			1.0f};

		int n = 256;
		LinearTransform i2t( 0, n, 0, 1 );

		DrawText( {0, -0.1f, 0}, "Arc Length Parametarization" );
		DrawText( {0, -0.1f, -2}, "Naive" );

		// Bad Case
		glm::vec3 offset( 0, 0, -2 );
		for ( int i = 0; i < n; ++i )
		{
			float t0 = i2t( i );
			float t1 = i2t( i + 1 );

			glm::vec3 a = bezier( ps, t0 );
			glm::vec3 b = bezier( ps, t1 );

			glm::vec3 d = dbezier( ps, t0 );

			DrawLine( a + offset, b + offset, {255, 64, 64} );

			float R = glm::mix( rs[0], rs[1], t0 );
			DrawCircle( a + offset, glm::normalize( d ), {255, 64, 64}, R );
		}

		// Good Case
		for ( int i = 0; i < n; ++i )
		{
			float t0 = i2t( i );
			float t1 = i2t( i + 1 );

			glm::vec3 a = bezier( ps, t0 );
			glm::vec3 b = bezier( ps, t1 );

			glm::vec3 d = dbezier( ps, t0 );

			DrawLine( a, b, {255, 255, 255} );

			// float l_01 = bezierLengthN10(ps, t0) / bezierLengthN10(ps, 1.0f);
			float l_01 = bezier( vs, t0 );
			float R = glm::mix( rs[0], rs[1], l_01 );
			DrawCircle( a, glm::normalize( d ), {255, 255, 255}, R );
		}

		// Gaussian Quadrature Check
		float ts[3] = {1.0f / 3.0f, 2.0f / 3.0f, 1.0f};
		float Ls[3] = {};

		for ( int j = 0; j < 3; ++j )
		{
			for ( int i = 0; i < n; ++i )
			{
				float t0 = i2t( i ) * ts[j];
				float t1 = i2t( i + 1 ) * ts[j];

				glm::vec3 a = bezier( ps, t0 );
				glm::vec3 b = bezier( ps, t1 );

				glm::vec3 d = dbezier( ps, t0 );

				DrawLine( a, b, {255, 255, 255} );

				Ls[j] += glm::distance( a, b );
			}
		}

		PopGraphicState();
		EndCamera();

		BeginImGui();

		ImGui::SetNextWindowSize( {500, 800}, ImGuiCond_Once );
		ImGui::Begin( "Panel" );
		ImGui::Text( "fps = %f", GetFrameRate() );
		ImGui::SliderFloat2( "radii", rs, 0.0f, 1.0f );

		for ( int j = 0; j < 3; ++j )
		{
			ImGui::Text( "%.6f [%d], n3=%f", Ls[j], j, bezierLengthN10( ps, ts[j] ) );
		}

		ImGui::End();

		EndImGui();
	}

	pr::CleanUp();
}
