// Copyright 2019-present Nathan Rusch
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef HDNUKE_HYDRAOPBASES_H
#define HDNUKE_HYDRAOPBASES_H

#include <pxr/pxr.h>

#include <pxr/imaging/hd/changeTracker.h>
#include <pxr/imaging/hd/types.h>

#include <DDImage/Op.h>


using namespace DD::Image;

PXR_NAMESPACE_OPEN_SCOPE


class HydraPrimOpManager;

class HydraPrimOp
{
public:
    HydraPrimOp() { }
    virtual ~HydraPrimOp();

    virtual const TfToken& GetPrimTypeName() const = 0;

    virtual void Populate(HydraPrimOpManager* manager) = 0;

    inline HdDirtyBits GetDirtyBits() const { return _dirtyBits; }
    inline bool IsDirty() const { return _dirtyBits != HdChangeTracker::Clean; }
    inline void MarkDirty(HdDirtyBits bits = HdChangeTracker::AllDirty) {
        _dirtyBits = bits;
    }
    inline void MarkClean() { _dirtyBits = HdChangeTracker::Clean; }

private:
    HdDirtyBits _dirtyBits = HdChangeTracker::Clean;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDNUKE_HYDRAOPBASES_H
