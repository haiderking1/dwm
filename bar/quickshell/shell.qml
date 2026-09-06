//@ pragma UseQApplication

import Quickshell
import "bar"

ShellRoot {
    // Quickshell owns file watching, hot reload, and per-screen window teardown.
    Variants {
        model: Quickshell.screens
        Bar {
            required property ShellScreen modelData
            screen: modelData
        }
    }
}
