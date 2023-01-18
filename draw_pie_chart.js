google.charts.load('current', {'packages':['corechart']});
google.charts.setOnLoadCallback(drawAllChart);

function drawChart(tit, arr, id) {

    var data = google.visualization.arrayToDataTable(arr);

    var options = {
        title: tit
    };

    var chart = new google.visualization.PieChart(document.getElementById(id));

    chart.draw(data, options);
}

function drawAllChart() {
    // parser output
 }
