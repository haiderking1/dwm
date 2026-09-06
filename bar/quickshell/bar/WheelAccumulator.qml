import QtQuick

QtObject {
    property real remainder: 0

    function take(delta) {
        remainder += delta;
        const steps = remainder < 0 ? Math.ceil(remainder / 120) : Math.floor(remainder / 120);
        remainder -= steps * 120;
        return steps;
    }
}
