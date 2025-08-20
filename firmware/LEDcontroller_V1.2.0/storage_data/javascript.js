/*** GLOBAL VARIABLES *****************************************************************/
// Device configuration
var rgb_outputs;
var w_outputs;

/*** FUNCTIONS ************************************************************************/

function get_conf() {
    let xhr = new XMLHttpRequest();
    xhr.open('GET', '/api/conf', true);
    xhr.onload = function () {
        if (xhr.status === 200) {
            let response_raw = xhr.responseText;
            //console.log(response_raw);

            let response_json = JSON.parse(response_raw)
            w_outputs = parseInt(response_json.w_outputs);
            rgb_outputs = parseInt(response_json.rgb_outputs);
            generate_sliders();
            get_channel_values();
        }
    };
    xhr.send();

    // Set a timeout to detect no response
    setTimeout(function () {
        if (xhr.readyState !== 4) {
            xhr.abort();
        }
    }, 3000); // 3 sec
}

function get_channel_values() {
    let xhr = new XMLHttpRequest();
    xhr.open('GET', '/api/ch/all', true);
    xhr.onload = function () {
        if (xhr.status === 200) {
            let response_raw = xhr.responseText;
            //console.log(response_raw);

            let response_json = JSON.parse(response_raw)

            // DUTY values
            for (let ch_cnt = 0; ch_cnt < w_outputs + (rgb_outputs * 3); ch_cnt++) {
                let slider_tmp = document.getElementById(`id_slider_ch${ch_cnt}`);
                let value_tmp = parseInt(response_json[`ch${ch_cnt}_duty`]);
                slider_tmp.value = value_tmp;
                update_num(ch_cnt, value_tmp);
            }

            // ENABLE values
            for (let rgb_cnt = 0; rgb_cnt < rgb_outputs; rgb_cnt++) {
                let element_tmp = document.getElementById(`id_checkbox_rgb${rgb_cnt}`);

                let enable_tmp = 1;
                enable_tmp &= parseInt(response_json[`ch${3 * rgb_cnt + 0}_enable`]);
                enable_tmp &= parseInt(response_json[`ch${3 * rgb_cnt + 1}_enable`]);
                enable_tmp &= parseInt(response_json[`ch${3 * rgb_cnt + 2}_enable`]);
                element_tmp.checked = enable_tmp;
                if (enable_tmp != 1) {
                    //console.log(rgb_cnt, value)
                    post_enable_rgb(rgb_cnt, false);
                }
            }
            for (let w_cnt = rgb_outputs * 3; w_cnt < w_outputs + (rgb_outputs * 3); w_cnt++) {
                let element_tmp = document.getElementById(`id_checkbox_ch${w_cnt}`);
                element_tmp.checked = parseInt(response_json[`ch${w_cnt}_enable`]);
            }
        }
    };
    xhr.send();

    // Set a timeout to detect no response
    setTimeout(function () {
        if (xhr.readyState !== 4) {
            xhr.abort();
        }
    }, 3000); // 3 sec
}

function generate_sliders() {
    let rgb_cnt = 0;
    for (rgb_cnt = 0; rgb_cnt < rgb_outputs; rgb_cnt++) {
        generate_rgb_slider(rgb_cnt);
    }
    let w_cnt = rgb_cnt * 3;
    for (; w_cnt < w_outputs + (rgb_outputs * 3); w_cnt++) {
        generate_w_slider(w_cnt);
    }
}

function generate_w_slider(ch_num) {
    // ch_num = 0, 1, 2, ...
    let html = `<div class="class_white">` +
        `<label class="switch">` +
        `<input id="id_checkbox_ch${ch_num}" type="checkbox" onclick="post_enable(${ch_num}, this.checked)">` +
        `<span class="slider round"></span>` +
        `</label>` +
        `<p><span id="id_value_ch${ch_num}">99</span></p>` +
        `<input class="slider_grey" id="id_slider_ch${ch_num}" type="range" value="" min="0" max="1023"` +
        `onchange="post_duty(${ch_num}, this.value)" oninput="update_num(${ch_num}, this.value)">` +
        `</div>` +
        `<br>`;
    document.body.innerHTML += html;
}

function generate_rgb_slider(rgb_num) {
    // rgb_num = 0, 1, 2, ...
    let html = `<div class="class_rgb">` +
        `<p>` +
        `<span id="id_value_ch${3 * rgb_num + 0}" style="color:#ff2020">99</span>&nbsp;` +
        `<span id="id_value_ch${3 * rgb_num + 1}" style="color:#66ff66">99</span>&nbsp;` +
        `<span id="id_value_ch${3 * rgb_num + 2}" style="color:#0060ff">99</span>` +
        `</p>` +
        `<input class="slider_red" id="id_slider_ch${3 * rgb_num + 0}" type="range" value="" min="0" max="1023"` +
        `onchange="post_duty(${3 * rgb_num + 0}, this.value)" oninput="update_num(${3 * rgb_num + 0}, this.value)">` +
        `<input class="slider_green" id="id_slider_ch${3 * rgb_num + 1}" type="range" value="" min="0" max="1023"` +
        `onchange="post_duty(${3 * rgb_num + 1}, this.value)" oninput="update_num(${3 * rgb_num + 1}, this.value)">` +
        `<input class="slider_blue" id="id_slider_ch${3 * rgb_num + 2}" type="range" value="" min="0" max="1023"` +
        `onchange="post_duty(${3 * rgb_num + 2}, this.value)" oninput="update_num(${3 * rgb_num + 2}, this.value)">` +
        `<label class="switch">` +
        `<input id="id_checkbox_rgb${rgb_num}" type="checkbox" onclick="post_enable_rgb(${rgb_num}, this.checked)">` +
        `<span class="slider round"></span>` +
        `</label>` +
        `</div>` +
        `<br>`;
    document.body.innerHTML += html;
}

function post_duty(ch_num, value) {
    let xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/ch', true);
    xhr.setRequestHeader('Content-Type', 'application/json');

    let post_data = `{ "ch${ch_num}_duty":${value} }`;

    xhr.onload = function () {
        if (xhr.status === 200) {
            let response_raw = xhr.responseText;
            //console.log(response_raw);
        }
        else {
            //console.log("Unknown error");
        }
    };
    xhr.send(post_data);

    // Set a timeout to detect no response
    setTimeout(function () {
        if (xhr.readyState !== 4) {
            xhr.abort();
        }
    }, 5000); // 5 sec

}

function update_num(ch_num, value) {
    let num_tmp = document.getElementById(`id_value_ch${ch_num}`);
    num_tmp.innerHTML = Math.ceil(value / 10.23);
}

function post_enable_rgb(rgb_num, value) {
    //console.log(rgb_num, value)
    for (let cnt = rgb_num * 3; cnt < ((rgb_num + 1) * 3); cnt++) {
        post_enable(cnt, value);
    }
}

function post_enable(ch_num, value) {
    let value_int;
    if (value == true) {
        value_int = 1;
    }
    else {
        value_int = 0;
    }
    //console.log(ch_num, value, value_int)

    let xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/ch', true);
    xhr.setRequestHeader('Content-Type', 'application/json');

    let post_data = `{ "ch${ch_num}_enable":${value_int} }`;

    xhr.onload = function () {
        if (xhr.status === 200) {
            let response_raw = xhr.responseText;
            //console.log(response_raw);
        }
        else {
            //console.log("Unknown error");
        }
    };
    xhr.send(post_data);

    // Set a timeout to detect no response
    setTimeout(function () {
        if (xhr.readyState !== 4) {
            xhr.abort();
        }
    }, 5000); // 5 sec

}

/*** ON WEBPAGE RELOAD ****************************************************************/

window.onload = function () {
    get_conf();
};

/*** END ******************************************************************************/