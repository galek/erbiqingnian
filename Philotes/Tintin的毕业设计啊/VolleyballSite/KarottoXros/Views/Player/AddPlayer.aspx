<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
<script src="../../Scripts/jquery-1.4.1.min.js" type="text/javascript"></script>
    <title></title>
</head>
<style type="text/css">
<!--
body {
	background-image: url(../../image/bg1.png);
	background-repeat: repeat-x;
}
div#wrap {
    height:45px;
    text-align:center;
 }
div#title {
    text-align:center;
    margin-bottom:30px;
 }
div#content {
    vertical-align:middle;
    display:table-cell;
    border:1px solid #FF0099;
    background-color:#FFCCFF;
    width:760px;
 }
-->
</style>
<body>
    <div id="title"><img src="../../image/ba2.png" width="704" height="69" border="0" /></div>
    <div>
    <%using(Html.BeginForm("AddPlayerAction","Player")){ %>
    <div id="wrap">球员姓名<%=Html.TextBox("Name") %></div>
    <div id="wrap">中文姓名<%=Html.TextBox("ChineseName") %></div>
    <div id="wrap">身高<%=Html.TextBox("Hight") %></div>
    <div id="wrap">体重<%=Html.TextBox("Weight") %></div>
    <div id="wrap">生日<%=Html.DropDownList("Year") %>年<%=Html.DropDownList("Mouth") %>月<%=Html.DropDownList("Day") %></div>
    <div id="wrap">大学<%=Html.TextBox("University")%></div>
    <!--
    <div>第一位置<%=Html.DropDownList("Loc") %></div>
    <div>擅长位置：<div>控球后卫<%=Html.CheckBox("PG",false,null) %></div><div>得分后位<%=Html.CheckBox("SG",false,null) %></div><div>小前锋<%=Html.CheckBox("SF",false,null) %></div><div>大前锋<%=Html.CheckBox("PF",false,null) %></div><div>中锋<%=Html.CheckBox("C",false,null) %></div></div>
    -->
    <div id="wrap">简介<%=Html.TextArea("Info")%>
    </div>
    <div id="wrap">
    <input type="submit" value="提交" />
    </div>
    <%} %>
    </div>

    <script language="javascript" type="text/javascript">

        $("#Year").change(function (data) {
            var years = $("#Year").val();
            var mouth = $("#Mouth").val();
            var day;
            switch (mouth) {
                case "1":
                case "3":
                case "5":
                case "7":
                case "8":
                case "10":
                case "12": day = 31; break;

            }

            switch (mouth) {
                case "4":
                case "6":
                case "9":
                case "11":
                case "12": day = 30; break;

            }


            if (mouth == "2") {
                if (years % 4 == 0) {
                    day = 29;
                } else {
                    day = 28;
                }
            }
            var days = new Array(day);
            $("#Day").empty();
            for (var i = 1; i <= day; i++) {
                $("<option>" + i + "</option>").appendTo("#Day");
            }

        });
    </script>

        <script language="javascript" type="text/javascript">

            $("#Mouth").change(function (data) {
                var years = $("#Year").val();
                var mouth = $("#Mouth").val();
                var day;
                switch (mouth) {
                    case "1":
                    case "3":
                    case "5":
                    case "7":
                    case "8":
                    case "10":
                    case "12": day = 31; break;

                }

                switch (mouth) {
                    case "4":
                    case "6":
                    case "9":
                    case "11":
                    case "12": day = 30; break;

                }


                if (mouth == "2") {
                    if (years % 4 == 0) {
                        day = 29;
                    } else {
                        day = 28;
                    }
                }

                var days = new Array(day);
                $("#Day").empty();
                for (var i = 1; i <= day; i++) {
                    $("<option>" + i + "</option>").appendTo("#Day");
                }

            });
    </script>
</body>
</html>
