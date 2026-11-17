#ifndef LAB2_ITEM_H
#define LAB2_ITEM_H

struct Item {
    int volume;
    int value;

    Item(int volume, int value): volume(volume), value(value) {}

    Item() : volume(0), value(0) {}
};

#endif
