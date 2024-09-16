<?php
header("Access-Control-Allow-Origin: *");
header("Content-Type: application/json; charset=UTF-8");

include_once '../config/config.php';
include_once '../class/data_log.php';

$database = new Database();
$db = $database->getConnection();

$item = new Device_log($db);

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    // The request is using the POST method
    $data = json_decode(file_get_contents("php://input"));
    $item->ph = $data->ph;
    $item->tds = $data->tds;
    $item->suhu = $data->suhu;
} elseif ($_SERVER['REQUEST_METHOD'] === 'GET') {
    // The request is using the GET method
    $item->ph = isset($_GET['ph']) ? $_GET['ph'] : die('wrong structure!');
    $item->tds = isset($_GET['tds']) ? $_GET['tds'] : die('wrong structure!');
    $item->suhu = isset($_GET['suhu']) ? $_GET['suhu'] : die('wrong structure!');
} else {
    die('wrong request method');
}

if ($item->createLogData()) {
    echo 'Data created successfully.';
} else {
    echo 'Data could not be created.';
}
