import {render} from 'inferno';
import Component from 'inferno-component';
import {Incrementer} from './components/Incrementer';

const container = document.getElementById('app');

class MyComponent extends Component<any, any> {
	public state: {visible: boolean};

	constructor(props, context) {
		super(props, context);

		this.state.visible = true;
	}

	toggleVisibility = () => {
		console.log('>>> toggle');
		this.setState({
			visible: !this.state.visible
		});
	}

	maybeShowApp = () => {
		if (this.state.visible) {
			return <Incrementer name={'Crazy button'}/>
		}
	}

	render() {
		return (
			<div>
				<button onClick={this.toggleVisibility}>Toggle Visibility</button>
				{this.maybeShowApp()}
			</div>
		);
	}
}

render(<MyComponent />, container);
