import QtQuick
import QtTest
import "../services/Protocol.js" as Protocol

TestCase {
    name: "DwmProtocol"

    function monitor(num, x, y, selected, occupied, urgent) {
        return {num: num, x: x, y: y, width: 1920, height: 1080,
            selected: selected, occupied: occupied, urgent: urgent,
            layout: "[]=", title: "Test window", fullscreen: false};
    }

    function test_stateAndMemory() {
        const state = {tags: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"],
            selectedMonitor: 7, monitors: [monitor(7, 1920, 0, 512, 1, 4)]};
        compare(Protocol.decode(JSON.stringify({type: "state", data: state})).data, state);
        compare(Protocol.decode('{"type":"memory","percent":23,"usedGiB":4.2}').usedGiB, 4.2);
        compare(Protocol.tagLabel(state, 10), "10");
        compare(Protocol.tagLabel(state, 3), "3");
    }

    function test_disconnectAndUnknown() {
        compare(Protocol.decode('{"type":"state","data":null}').data, null);
        compare(Protocol.decode('{"type":"future"}'), null);
        compare(Protocol.monitorForScreen(null, {x: 0, y: 0, width: 1920, height: 1080}), null);
        compare(Protocol.visibleTags(null), []);
    }

    function test_invalid_data() {
        return [
            {tag: "bad JSON", line: "{"},
            {tag: "null envelope", line: "null"},
            {tag: "missing state", line: '{"type":"state"}'},
            {tag: "missing memory", line: '{"type":"memory"}'},
            {tag: "bad percent", line: '{"type":"memory","percent":101,"usedGiB":1}'},
            {tag: "negative memory", line: '{"type":"memory","percent":1,"usedGiB":-1}'},
            {tag: "bad monitor", line: '{"type":"state","data":{"tags":[],"selectedMonitor":0,"monitors":[{}]}}'}
        ];
    }

    function test_invalid(data) {
        let rejected = false;
        try { Protocol.decode(data.line); } catch (error) { rejected = true; }
        verify(rejected);
    }

    function test_screenGeometryNotIndex() {
        const first = monitor(7, 1920, 0, 1, 0, 0);
        const second = monitor(2, 0, 0, 2, 0, 0);
        const state = {monitors: [first, second]};
        compare(Protocol.monitorForScreen(state, {x: 0, y: 0, width: 1920, height: 1080}).num, 2);
        compare(Protocol.monitorForScreen(state, {x: 1920, y: 0, width: 1920, height: 1080}).num, 7);
        compare(Protocol.monitorForScreen(state, {x: 0, y: 0, width: 1280, height: 720}), null);
        compare(Protocol.monitorForScreen({monitors: [first, first]}, first), null);
    }

    function test_dynamicTags() {
        compare(Protocol.visibleTags(monitor(0, 0, 0, 512, 5, 32)), [1, 3, 6, 10]);
        compare(Protocol.visibleTags(monitor(0, 0, 0, 8, 0, 0)), [4]);
        compare(Protocol.visibleTags(monitor(0, 0, 0, 1023, 0, 0)), [1,2,3,4,5,6,7,8,9,10]);
    }

    function test_selectedBitNavigation() {
        compare(Protocol.selectedIndex(512), 10);
        compare(Protocol.nextTag(512, 1), 1);
        compare(Protocol.nextTag(1, -1), 10);
        compare(Protocol.nextTag(8, 1), 5);
        compare(Protocol.nextTag(8, -1), 3);
        compare(Protocol.nextTag(8 | 32, 1), 5);
        compare(Protocol.nextTag(0, 1), 0);
        compare(Protocol.nextTag(1, 12), 3);
    }
}
