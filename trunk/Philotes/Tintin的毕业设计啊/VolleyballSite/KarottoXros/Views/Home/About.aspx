<%@ Page Language="C#" MasterPageFile="~/Views/Shared/Site.Master" Inherits="System.Web.Mvc.ViewPage" %>

<asp:Content ID="aboutTitle" ContentPlaceHolderID="TitleContent" runat="server">
    关于我们
</asp:Content>

<asp:Content ID="aboutContent" ContentPlaceHolderID="MainContent" runat="server">
    <h2>关于</h2>
    <p>
        <%=Html.ActionLink("连过去","Login","KXros",null,null) %>
        <%=Html.ActionLink("连过去2","Index","KXros",null,null) %>
    </p>
</asp:Content>
