// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

class TestHelpers
{
public:
    static void DisableNetwork();
    static void EnableNetwork();

private:
    // Disallow creating an instance of this object
    TestHelpers() {}
};
