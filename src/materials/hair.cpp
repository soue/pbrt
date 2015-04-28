
/*
    pbrt source code Copyright(c) 1998-2012 Matt Pharr and Greg Humphreys.

    This file is part of pbrt.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */


// materials/hair.cpp*
#include "stdafx.h"
#include "materials/hair.h"
#include "paramset.h"
#include "reflection.h"
#include "diffgeom.h"
#include "texture.h"

// HairMaterial Method Definitions
BSDF *HairMaterial::GetBSDF(const DifferentialGeometry &dgGeom,
                             const DifferentialGeometry &dgShading,
                             MemoryArena &arena) const {
    // Allocate _BSDF_, possibly doing bump mapping with _bumpMap_
    DifferentialGeometry dgs;
    if (bumpMap)
        Bump(bumpMap, dgGeom, dgShading, &dgs);
    else
        dgs = dgShading;
    BSDF *bsdf = BSDF_ALLOC(arena, BSDF)(dgs, dgGeom.nn);

    // Evaluate textures for _HairMaterial_ material and allocate BRDF
    
    Spectrum kd = Kd->Evaluate(dgs).Clamp();
    if (!kd.IsBlack()) {
        bsdf->Add(BSDF_ALLOC(arena, Lambertian)(kd));
    }
    Spectrum ks = Ks->Evaluate(dgs).Clamp();
    if (!ks.IsBlack()) {
	Fresnel *fresnel = BSDF_ALLOC(arena, FresnelDielectric)(1.5f, 1.f);
        float rough = roughness->Evaluate(dgs);
        BxDF *spec = BSDF_ALLOC(arena, Microfacet)
                       (ks, fresnel, BSDF_ALLOC(arena, Blinn)(1.f / rough));
	
        bsdf->Add(BSDF_ALLOC(arena, Lambertian)(ks));
    }
    
    return bsdf;
}


HairMaterial *CreateHairMaterial(const Transform &xform,
        const TextureParams &mp) {
    Reference<Texture<Spectrum> > Kd = mp.GetSpectrumTexture("Kd", Spectrum(0.5f));
    Reference<Texture<Spectrum> > Ks = mp.GetSpectrumTexture("Ks", Spectrum(0.5f));
    Reference<Texture<float> > roughness = mp.GetFloatTexture("roughness", 0.5f);
    Reference<Texture<float> > bumpMap = mp.GetFloatTextureOrNull("bumpmap");
    return new HairMaterial(Kd, Ks, roughness, bumpMap);
}


