#pragma once

const char htmlsrc1[] =
    R"=====(
<html>

<head>
    <title>
        Lights driver
    </title>

    <style>
        body {
            background-color: black;
            color: blanchedalmond;
            padding: 1em;
        }

        .btn {
            width: 100%;
            height: 2em;
            font-size: 3em;
        }

        .range {
            width: 100%;
        }

        input[type=range] {
            height: 100px;
            -webkit-appearance: none;
            margin: 10px 0;
            width: 100%;
        }

        input[type=range]:focus {
            outline: none;
        }

        input[type=range]::-webkit-slider-runnable-track {
            width: 100%;
            height: 100px;
            cursor: pointer;
            animate: 0.2s;
            box-shadow: 1px 1px 1px #000000;
            background: #3071A9;
            border-radius: 5px;
            border: 1px solid #000000;
        }

        input[type=range]::-webkit-slider-thumb {
            box-shadow: 1px 1px 1px #000000;
            border: 1px solid #000000;
            height: 100px;
            width: 50px;
            border-radius: 15px;
            background: #FFFFFF;
            cursor: pointer;
            -webkit-appearance: none;
            margin-top: 0px;
        }

        input[type=range]:focus::-webkit-slider-runnable-track {
            background: #3071A9;
        }

        input[type=range]::-moz-range-track {
            width: 100%;
            height: 100px;
            cursor: pointer;
            animate: 0.2s;
            box-shadow: 1px 1px 1px #000000;
            background: #3071A9;
            border-radius: 15px;
            border: 1px solid #000000;
        }

        input[type=range]::-moz-range-thumb {
            box-shadow: 1px 1px 1px #000000;
            border: 1px solid #000000;
            height: 100px;
            width: 50px;
            border-radius: 15px;
            background: #FFFFFF;
            cursor: pointer;
        }

        input[type=range]::-ms-track {
            width: 100%;
            height: 100px;
            cursor: pointer;
            animate: 0.2s;
            background: transparent;
            border-color: transparent;
            color: transparent;
        }

        input[type=range]::-ms-fill-lower {
            background: #3071A9;
            border: 1px solid #000000;
            border-radius: 10px;
            box-shadow: 1px 1px 1px #000000;
        }

        input[type=range]::-ms-fill-upper {
            background: #3071A9;
            border: 1px solid #000000;
            border-radius: 10px;
            box-shadow: 1px 1px 1px #000000;
        }

        input[type=range]::-ms-thumb {
            margin-top: 1px;
            box-shadow: 1px 1px 1px #000000;
            border: 1px solid #000000;
            height: 100px;
            width: 50px;
            border-radius: 15px;
            background: #FFFFFF;
            cursor: pointer;
        }

        input[type=range]:focus::-ms-fill-lower {
            background: #3071A9;
        }

        input[type=range]:focus::-ms-fill-upper {
            background: #3071A9;
        }

        .checkbox {
            height: 50px;
            width: 50px;
        }

        .label {
            font-size: 3em;
        }

        h1 {
            font-size: 4em;
        }

        .number {
            height: 50px;
            font-size: 3em;
        }

        .settings {
            margin-top: 3em;
        }
    </style>
</head>
<body>
)=====";

const char htmlsrc2[] =
    R"=====(    
        <input class="btn" type="button" value="SAVE" onclick="save()">
    </section>

    <script>
        function turnOnOff(id, on) {
            const data = { id: id, value: !!on, local: true };

            fetch("/onoff", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(data)
            });
        }

        function changeBrightness(id, value) {
            const data = { id: id, value: value, local: true };

            fetch("/brightness", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(data)
            });
        }

        function save() {
            const from = document.querySelector('input[id="from"]');
            const to = document.querySelector('input[id="to"]');
            const data = { from: from.value, to: to.value, local: true };

            fetch("/save", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(data)
            });
        }

        var checkboxes = document.querySelectorAll('input[type="checkbox"]');

        function changeAuto(id) {
            var checked = checkboxes[id - 1].checked;
            const data = { id: id, value: checked ? 1 : 0, local: true };

            fetch("/auto", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(data)
            });
        }
    </script>
</body>

</html>
)=====";