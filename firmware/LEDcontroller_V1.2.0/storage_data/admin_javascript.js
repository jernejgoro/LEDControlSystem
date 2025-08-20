/*** FUNCTIONS ************************************************************************/

function show_cred() {
    let checkbox_enable_wifi = document.getElementById("id_checkbox_cred");
    let ssid_field = document.getElementById("id_ssid");
    let pass_field = document.getElementById("id_pass");

    if (checkbox_enable_wifi.checked == true) {
        ssid_field.type = "text";
        pass_field.type = "text";
    }
    else {
        ssid_field.type = "password";
        pass_field.type = "password";
    }
}

function set_current_values() {
    let xhr = new XMLHttpRequest();
    xhr.open('GET', '/api/conf', true);
    xhr.onload = function () {
        if (xhr.status === 200) {
            let response_raw = xhr.responseText;
            console.log(response_raw);

            let response_json = JSON.parse(response_raw)

            let w_outpus = document.getElementById("id_w_outputs");
            let rgb_outpus = document.getElementById("id_rgb_outputs");
            let checkbox_enable_wifi = document.getElementById("id_checkbox_enable_wifi");

            w_outpus.value = parseInt(response_json.w_outputs);
            rgb_outpus.value = parseInt(response_json.rgb_outputs);
            checkbox_enable_wifi.checked = parseInt(response_json.enable_wifi);
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

function post_reboot() {

    let xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/reboot', true);
    xhr.setRequestHeader('Content-Type', 'application/json');

    let feedback_manage_element = document.getElementById("feedback_manage");
    feedback_manage_element.innerText = "Sending reboot command";

    xhr.onload = function () {
        if (xhr.status === 200) {
            let response_raw = xhr.responseText;
            //console.log(response_raw);
            feedback_manage_element.innerText = response_raw;
        }
        else {
            feedback_manage_element.innerText = "Unknown error";
        }
    };
    xhr.send();

    // Set a timeout to detect no response
    setTimeout(function () {
        if (xhr.readyState !== 4) {
            xhr.abort();
            feedback_manage_element.innerText = "ERROR: No response received";
        }
        else {
            location.reload();
        }
    }, 10000); // 10 sec
}

function post_conf() {
    let xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/conf', true);
    xhr.setRequestHeader('Content-Type', 'application/json');

    let feedback_manage_element = document.getElementById("feedback_conf");
    feedback_manage_element.innerText = "Sending configuration";

    let w_outputs = parseInt(document.getElementById("id_w_outputs").value);
    let rgb_outputs = parseInt(document.getElementById("id_rgb_outputs").value);
    let checkbox_enable_wifi;
    if (document.getElementById("id_checkbox_enable_wifi").checked == true) {
        checkbox_enable_wifi = 1;
    }
    else {
        checkbox_enable_wifi = 0;
    }

    let post_data = `{ "w_outputs":${w_outputs} , "rgb_outputs":${rgb_outputs} , "enable_wifi":${checkbox_enable_wifi} }`;

    xhr.onload = function () {
        if (xhr.status === 200) {
            let response_raw = xhr.responseText;
            //console.log(response_raw);
            feedback_manage_element.innerText = response_raw;
        }
        else {
            feedback_manage_element.innerText = "Unknown error";
        }
    };
    xhr.send(post_data);

    // Set a timeout to detect no response
    setTimeout(function () {
        if (xhr.readyState !== 4) {
            xhr.abort();
            feedback_manage_element.innerText = "ERROR: No response received";
        }
    }, 10000); // 10 sec
}

function post_cred() {
    let xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/cred', true);
    xhr.setRequestHeader('Content-Type', 'application/json');

    let feedback_manage_element = document.getElementById("feedback_cred");
    feedback_manage_element.innerText = "Sending credentials";

    let ssid = document.getElementById("id_ssid").value;
    let pass = document.getElementById("id_pass").value;
    let post_data = `{ "WIFI_SSID":"${ssid}" , "WIFI_PASSWORD":"${pass}" }`;

    xhr.onload = function () {
        if (xhr.status === 200) {
            let response_raw = xhr.responseText;
            //console.log(response_raw);
            feedback_manage_element.innerText = response_raw;
        }
        else {
            feedback_manage_element.innerText = "Unknown error";
        }
    };
    xhr.send(post_data);

    // Set a timeout to detect no response
    setTimeout(function () {
        if (xhr.readyState !== 4) {
            xhr.abort();
            feedback_manage_element.innerText = "ERROR: No response received";
        }
    }, 10000); // 10 sec
}

/*** ON WEBPAGE RELOAD ****************************************************************/

window.onload = function () {
    // Hide credentials
    document.getElementById("id_checkbox_cred").checked = false;
    show_cred();
    // Set current values in "configuration" section
    set_current_values();

    // Label text in search box when clicked (EVENT LISTENER) 
    const input_ssid = document.getElementById("id_ssid");
    input_ssid.addEventListener('focus', function (e) {
        input_ssid.select()
    })
    const input_pass = document.getElementById("id_pass");
    input_pass.addEventListener('focus', function (e) {
        input_pass.select()
    })
    const input_w = document.getElementById("id_w_outputs");
    input_w.addEventListener('focus', function (e) {
        input_w.select()
    })
    const input_rgb = document.getElementById("id_rgb_outputs");
    input_rgb.addEventListener('focus', function (e) {
        input_rgb.select()
    })
};

/*** END ******************************************************************************/