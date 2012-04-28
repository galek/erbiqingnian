<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" >
<head runat="server">
<script src="../../Scripts/jquery-1.4.1.min.js" type="text/javascript"></script>
    <title></title>
</head>
<body>
    <div>
    <%using (Html.BeginForm("AddAction", "Event"))
      { %>
    <div>主队<%=Html.DropDownList("TeamListHost")%></div>
    <div>客队<%=Html.DropDownList("TeamListVisit")%></div>
    <div>比赛日期<%=Html.DropDownList("Year")%>年<%=Html.DropDownList("Mouth")%>月<%=Html.DropDownList("Day")%></div>
    <div><input type="submit" value="提交" /></div>
    <%} %>
    </div>
      <script language="javascript" type="text/javascript">
          $(document).read(function () { });
    </script>

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
