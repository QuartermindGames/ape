# WLD v3 Format Specification

## Light

```
string className "test"
bool isHidden false
array float position { 0 0 0 }
array float colour { 0 0 0 0 }
float radius 0
float fov 0

float onIntensity 0
float onTime 0
float onTimeVariation 0

float offIntensity 0
float offTime 0
float offTimeVariation 0
```

## Portal

``` 
uint32 roomA
uint32 roomB

array float mins { 0 0 0 }
array float maxs { 0 0 0 }
```

## Room

```
array float mins { 0 0 0 }
array float maxs { 0 0 0 }

uint32 flags
```
