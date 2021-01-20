/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

const float fogFar = 11.0;
const float fogNear = 32.0;
uniform vec4 fogColour = vec4(0.50, 0.83, 1.0, 0.1);

vec4 CalculateFogTerm(vec4 n) {
    float fogDistance = (gl_FragCoord.z / gl_FragCoord.w) / (fogFar * 100.0);
    float fogAmount = 1.0 - fogDistance;
    fogAmount *= -(fogNear / 100.0);
    return mix(n, fogColour, clamp(fogAmount, 0.0, 1.0));
}
