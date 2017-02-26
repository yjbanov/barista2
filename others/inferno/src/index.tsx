import {render} from 'inferno';
import Component from 'inferno-component';
import {GiantApp} from './components/components';

const container = document.getElementById('app');

interface GiantAppWrapperState {
  visible: boolean;
}

class GiantAppWrapper extends Component<any, GiantAppWrapperState> {
	constructor(props, context) {
		super(props, context);
		this.state.visible = true;
	}

	toggleVisibility = () => {
		let before = window.performance.now();
		this.setState({
			visible: !this.state.visible
		});
		setTimeout(() => {
			let after = window.performance.now();
			console.log('>>> Render frame:', after - before, 'ms');
		}, 0);
	}

	render() {
		return (
			<div>
				<button onClick={this.toggleVisibility}>Toggle Visibility</button>
				{this.state.visible && <GiantApp/>}
			</div>
		);
	}
}

render(<GiantAppWrapper />, container);
