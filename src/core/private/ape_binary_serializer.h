// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_math.h>

typedef struct Serializer Serializer;

typedef enum SerializerMode
{
	SERIALIZER_MODE_READ,
	SERIALIZER_MODE_WRITE,
} SerializerMode;

typedef enum SerializerDataFormat
{
	SERIALIZER_DATA_FORMAT_I32,
	SERIALIZER_DATA_FORMAT_F32,
	SERIALIZER_DATA_FORMAT_STRING,
	SERIALIZER_DATA_FORMAT_VECTOR2,
	SERIALIZER_DATA_FORMAT_VECTOR3,
} SerializerDataFormat;

typedef void SerializerReadCallback( Serializer *serializer, void *stream );
typedef void SerializerWriteCallback( Serializer *serializer, void *stream );

// todo: make this an abstract interface; pass in read/write functions so we can use for networking etc.
Serializer *Serializer_Create( const char *path, SerializerMode mode );
void Serializer_Destroy( Serializer *serializer );

bool Serializer_ValidateDataFormat( Serializer *serializer, uint8_t target );

void Serializer_WriteI32( Serializer *serializer, int32_t var );
void Serializer_WriteF32( Serializer *serializer, float var );
void Serializer_WriteString( Serializer *serializer, const char *var );
void Serializer_WriteVector2( Serializer *serializer, const PLVector2 *var );
void Serializer_WriteVector3( Serializer *serializer, const PLVector3 *var );

int32_t Serializer_ReadI32( Serializer *serializer );
float Serializer_ReadF32( Serializer *serializer );
const char *Serializer_ReadString( Serializer *serializer, char *dst, size_t dstLength );
PLVector2 Serializer_ReadVector2( Serializer *serializer );
PLVector3 Serializer_ReadVector3( Serializer *serializer );

unsigned int Serializer_GetVersion( void );
