<?php

class Device_log
{
    // Connection
    private $conn;

    // Table
    private $db_table = "device1";

    // Columns
    public $id;
    public $ph;
    public $tds;
    public $suhu;
    public $created_at;

    // Db connection
    public function __construct($db)
    {
        $this->conn = $db;
    }

    // CREATE
    public function createLogData()
    {
        $sqlQuery = "INSERT INTO
                        " . $this->db_table . "
                    SET
                        ph = :ph, 
                        tds = :tds, 
                        suhu = :suhu";
        $stmt = $this->conn->prepare($sqlQuery);

        // sanitize
        $this->ph = htmlspecialchars(strip_tags($this->ph));
        $this->tds = htmlspecialchars(strip_tags($this->tds));
        $this->suhu = htmlspecialchars(strip_tags($this->suhu));

        // bind data
        $stmt->bindParam(":ph", $this->ph);
        $stmt->bindParam(":tds", $this->tds);
        $stmt->bindParam(":suhu", $this->suhu);

        if ($stmt->execute()) {
            return true;
        }
        return false;
    }
}
