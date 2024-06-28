import QtQuick

RoomsSwipeViewForm {

    previousItem.onTapped: {
        var prevIndex = swipeView.currentIndex - 1;
        if (prevIndex < 0)
            prevIndex = swipeView.count - 1
        swipeView.setCurrentIndex(prevIndex)

    }
    nextItem.onTapped: {
        var nextIndex = (swipeView.currentIndex + 1) % swipeView.count;
        swipeView.setCurrentIndex(nextIndex)
    }
}
