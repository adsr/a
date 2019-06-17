var Pattern = React.createClass({
  render: function() {
    return ( <div>patt</div> );
  }
});

var Instrument = Pattern;
var InstrumentDef = Pattern;
var Global = Pattern;

var Sequencer = React.createClass({
  getInitialState: function() {
    return {
      cur_page: 'patt'
    };
  },
  setPage: function(page) {
    this.setState({cur_page: page});
  },
  getPages: function() {
    return [
      { page: 'patt',     component: Pattern,       icon: 'th',     label: 'Patt' },
      { page: 'inst',     component: Instrument,    icon: 'stats',  label: 'Inst' },
      { page: 'inst_def', component: InstrumentDef, icon: 'wrench', label: 'InstDef' },
      { page: 'global',   component: Global,        icon: 'cog',    label: 'Global' }
    ];
  },
  render: function() {
    var page;
    var pages = this.getPages();
    for (var i = 0; i < pages.length; i++) {
      if (this.state.cur_page === pages[i].page) {
        page = React.createElement(pages[i].component, this.props);
        break;
      }
    }
    return (
      <div className="seq7f container">
        <nav className="navbar navbar-default">
          <div className="navbar-header navbar-brand">seq7f</div>
          <div className="navbar-collapse collapse">
            <ul className="nav navbar-nav"> {
              pages.map(function(p) {
                return (
                  <li className={this.state.cur_page == p.page ? 'active' : ''}>
                    <a href="#" onClick={this.setPage.bind(this, p.page)}    >
                      <span class="glyphicon glyphicon-{p.icon}"></span>
                      {p.label}
                    </a>
                  </li>
                );
              }, this)
            } </ul>
          </div>
        </nav>
        {page}
      </div>
    );
  }
});
