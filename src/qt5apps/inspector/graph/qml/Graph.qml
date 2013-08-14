import QtQuick 2.0
import QtQuick.Layouts 1.0

Flickable {
    id: the_root
    width: 500
    height: 500
    contentWidth: width;
    contentHeight: top_system.height

    signal clicked(string path)
    signal inputClicked(string path)
    signal outputClicked(string path)

    property var layoutComponents: MarSystemLayoutFactory {}
    property var systemViews: {"/": the_root}
    property var bugs: []

    MarSystemItem {
        id: top_system
        anchors {
            left: parent.left
            right: parent.right
        }
    }

    Behavior on contentY {
        animation: NumberAnimation {
            easing.type: Easing.OutCubic
        }
    }

    function navigateToItem( item ) {
        var item_pos = item.mapToItem(contentItem, 0, 0);
        the_root.contentY = item_pos.y;
    }

    Component.onCompleted: {
        top_system.setExpanded(true);
    }
}
