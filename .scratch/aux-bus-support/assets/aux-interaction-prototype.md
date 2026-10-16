# AUX interaction sketch

Throwaway outline for the existing straight-table editor. The owner approved this behavior in the live conversation.

```text
Outputs
Out 1 ... existing physical fields ...
...
Out 8 ... existing physical fields ...

AUX buses                              + AUX
Bus       Description                  Group          Remove
AUX 3     Filter modulation             Modulation     [x]
AUX 27    Stereo effect return L        Effects        [x]

Expander 1 ... existing physical rows ...
```

+ AUX opens the existing Material choice-dialog style with unused AUX buses 1 through 44. Choosing one adds a blank row. Rows stay in numeric order. Blank rows remain until removed and are stored with the preset. A populated row requires deletion confirmation. Cancel and Escape preserve it. A removed bus can be added again with empty fields. The AUX minimap uses numbered, neutral markers rather than cable colours.

Description and Group retain the existing 32-character printable-ASCII editing constraints. Compact layouts keep horizontal scrolling and accessible control targets.
